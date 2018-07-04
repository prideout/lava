#include <par/LavaLoader.h>

#include <par/LavaContext.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaDescCache.h>
#include <par/LavaGpuBuffer.h>
#include <par/LavaLog.h>
#include <par/LavaPipeCache.h>
#include <par/LavaSurfCache.h>

#include <par/AmberApplication.h>
#include <par/AmberProgram.h>

#include "vmath.h"

using namespace std;
using namespace par;

namespace {

struct Uniforms {
    float iResolution[4];
    float iTime;
};

struct Vertex {
    float position[2];
    uint32_t color;
};

const Vertex TRIANGLE_VERTICES[] {
    {{-1, -1}, 0xffff0000u},
    {{ 3, -1}, 0xff00ff00u},
    {{-1,  3}, 0xff0000ffu},
};

struct LavamacApp : AmberApplication {
    LavamacApp(SurfaceFn createSurface);
    ~LavamacApp();
    void draw(double seconds) override;
    LavaContext* mContext;
    AmberProgram* mOffscreenProgram;
    AmberProgram* mBackbufferProgram;
    LavaGpuBuffer* mVertexBuffer;
    LavaRecording* mRecording;
    LavaPipeCache* mPipelines;
    LavaDescCache* mDescriptors;
    LavaCpuBuffer* mUniforms[2];
    VkExtent2D mResolution;
    LavaSurfCache* mSurfaces;
    LavaSurface mOffscreenSurface;
    VkSampler mSampler;
};

static const std::string kBackbufferVs = AMBER_PREFIX_450 R"GLSL(
layout(location=0) in vec2 position;
layout(location=0) out highp vec2 vert_texcoord;
void main() {
    gl_Position = vec4(position, 0, 1);
    vert_texcoord = position.xy;
})GLSL";

static const std::string kBackbufferFs = AMBER_PREFIX_450 R"GLSL(
layout(location = 0) out vec4 frag_color;
layout(location = 0) in vec2 vert_texcoord;
layout(binding = 1) uniform sampler2D img;
void main() {
    frag_color = texture(img, vert_texcoord);
})GLSL";

static const std::string kToyVs = AMBER_PREFIX_450 R"GLSL(
layout(location=0) in vec2 position;
layout(location=1) in vec4 color;
layout(location=0) out highp vec2 vert_texcoord;
void main() {
    gl_Position = vec4(position, 0, 1);
    vert_texcoord = position.xy;
})GLSL";

static const std::string kToyFs = AMBER_PREFIX_450 R"GLSL(
// "2d signed distance functions" by Maarten
// https://www.shadertoy.com/view/4dfXDn

precision mediump int;
precision highp float;

layout(binding = 0) uniform ParamsBlock {
    vec4 iResolution;
    float iTime;
};

layout(location=0) out lowp vec4 frag_color;
layout(location=0) in highp vec2 vert_texcoord;

float pie(vec2 p, float angle) {
	angle = radians(angle) / 2.0;
	vec2 n = vec2(cos(angle), sin(angle));
	return abs(p).x * n.x + p.y*n.y;
}

float circleDist(vec2 p, float radius) {
	return length(p) - radius;
}

float triangleDist(vec2 p, float radius) {
	return max(	abs(p).x * 0.866025 + 
			   	p.y * 0.5, -p.y) 
				-radius * 0.5;
}

float triangleDist(vec2 p, float width, float height) {
	vec2 n = normalize(vec2(height, width / 2.0));
	return max(	abs(p).x*n.x + p.y*n.y - (height*n.y), -p.y);
}

float boxDist(vec2 p, vec2 size, float radius) {
	size -= vec2(radius);
	vec2 d = abs(p) - size;
  	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - radius;
}


float lineDist(vec2 p, vec2 start, vec2 end, float width) {
	vec2 dir = start - end;
	float lngth = length(dir);
	dir /= lngth;
	vec2 proj = max(0.0, min(lngth, dot((start - p), dir))) * dir;
	return length( (start - p) - proj ) - (width / 2.0);
}

///////////////////////
// Masks for drawing //
///////////////////////

float fillMask(float dist) {
	return clamp(-dist, 0.0, 1.0);
}

float innerBorderMask(float dist, float width) {
	float alpha1 = clamp(dist + width, 0.0, 1.0);
	float alpha2 = clamp(dist, 0.0, 1.0);
	return alpha1 - alpha2;
}

float outerBorderMask(float dist, float width) {
	float alpha1 = clamp(dist, 0.0, 1.0);
	float alpha2 = clamp(dist - width, 0.0, 1.0);
	return alpha1 - alpha2;
}

///////////////
// The scene //
///////////////

float sceneDist(vec2 p) {
	return circleDist(p, 40.0);
}

float sceneSmooth(vec2 p, float r) {
	float accum = sceneDist(p);
	accum += sceneDist(p + vec2(0.0, r));
	accum += sceneDist(p + vec2(0.0, -r));
	accum += sceneDist(p + vec2(r, 0.0));
	accum += sceneDist(p + vec2(-r, 0.0));
	return accum / 5.0;
}

//////////////////////
// Shadow and light //
//////////////////////

float shadow(vec2 p, vec2 pos, float radius) {
	vec2 dir = normalize(pos - p);
	float dl = length(p - pos);
	
	// fraction of light visible, starts at one radius (second half added in the end);
	float lf = radius * dl;
	
	// distance traveled
	float dt = 0.01;

	for (int i = 0; i < 64; ++i) {				
		// distance to scene at current position
		float sd = sceneDist(p + dir * dt);

        // early out when this ray is guaranteed to be full shadow
        if (sd < -radius) 
            return 0.0;
        
		// width of cone-overlap at light
		// 0 in center, so 50% overlap: add one radius outside of loop to get total coverage
		// should be '(sd / dt) * dl', but '*dl' outside of loop
		lf = min(lf, sd / dt);
		
		// move ahead
		dt += max(1.0, abs(sd));
		if (dt > dl) break;
	}

	// multiply by dl to get the real projected overlap (moved out of loop)
	// add one radius, before between -radius and + radius
	// normalize to 1 ( / 2*radius)
	lf = clamp((lf*dl + radius) / (2.0 * radius), 0.0, 1.0);
	lf = smoothstep(0.0, 1.0, lf);
	return lf;
}

vec4 drawLight(vec2 p, vec2 pos, vec4 color, float dist, float range, float radius) {
	// distance to light
	float ld = length(p - pos);
	
	// out of range
	if (ld > range) return vec4(0.0);
	
	// shadow and falloff
	float shad = shadow(p, pos, radius);
	float fall = (range - ld)/range;
	fall *= fall;
	float source = fillMask(circleDist(p - pos, radius));
	return (shad * fall + source) * color;
}

float luminance(vec4 col) {
	return 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
}

void setLuminance(inout vec4 col, float lum) {
	lum /= luminance(col);
	col *= lum;
}

void main() {
    vec2 fragCoord = vert_texcoord * iResolution.xy * 0.2 + vec2(128, 128);
	vec2 p = fragCoord + 0.5;
	vec2 c = iResolution.xy / 2.0;
    float dist = sceneDist(p);
	
	vec2 light2Pos = vec2(iResolution.x * (sin(3*iTime + 3.1415) + 1.2) / 7.0 - 128.0, 100.0);
	vec4 light2Col = vec4(1.0, 0.9, 0.8, 1.0);
	setLuminance(light2Col, 1.0);
		
	// gradient
	vec4 col = vec4(0.5, 0.5, 0.5, 1.0) * (1.0 - length(c - p)/iResolution.x);
	// light
	col += drawLight(p, light2Pos, light2Col, dist, 200.0, 8.0);
	// shape fill
	col = mix(col, vec4(0.2, 0.4, 0.6, 1.0), fillMask(dist));
	// shape outline
	col = mix(col, vec4(0.1, 0.1, 0.1, 1.0), innerBorderMask(dist, 1.5));

	frag_color = clamp(col, 0.0, 1.0);
})GLSL";

} // anonymous namespace

LavamacApp::LavamacApp(SurfaceFn createSurface) {
    // Create the instance, device, swap chain, and command buffers.
    mContext = LavaContext::create({
        .depthBuffer = false, .validation = true,
        .samples = VK_SAMPLE_COUNT_1_BIT, .createSurface = createSurface
    });
    const auto device = mContext->getDevice();
    const auto gpu = mContext->getGpu();
    const auto renderPass = mContext->getRenderPass();
    const auto extent = mContext->getSize();
    llog.info("Surface size: {}x{}", extent.width, extent.height);
    mResolution = extent;

    // Create offscreen surface.
    mSurfaces = LavaSurfCache::create(LavaSurfCache::Config { .device = device, .gpu = gpu });
    mOffscreenSurface = {
        .color = mSurfaces->createColorAttachment( LavaSurfCache::AttachmentConfig {
            .width = 512,
            .height = 512,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .enableUpload = false
        }),
        .colorLoad = VK_ATTACHMENT_LOAD_OP_DONT_CARE
    };

    // Begin populating a vertex buffer.
    mVertexBuffer = LavaGpuBuffer::create({
        .device = device, .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    auto stage = LavaCpuBuffer::create({
        .device = device, .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES), .source = TRIANGLE_VERTICES,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    });
    VkCommandBuffer cmdbuffer = mContext->beginWork();
    const VkBufferCopy region = { .size = sizeof(TRIANGLE_VERTICES) };
    vkCmdCopyBuffer(cmdbuffer, stage->getBuffer(), mVertexBuffer->getBuffer(), 1, &region);
    mSurfaces->finalizeAttachment(mOffscreenSurface.color, cmdbuffer);
    mContext->endWork();

    // Compile shaders.
    auto make_program = [device](string vshader, string fshader) {
        const string vs = AmberProgram::getChunk(__FILE__, vshader);
        const string fs = AmberProgram::getChunk(__FILE__, fshader);
        auto ptr = AmberProgram::create(vs, fs);
        if (!ptr->compile(device)) {
            delete ptr;
            ptr = nullptr; 
        }
        return ptr;
    };
    mOffscreenProgram = make_program("shadertoy.vs", "shadertoy.fs");
    mBackbufferProgram = make_program("backbuffer.vs", "backbuffer.fs");

    // Create the double-buffered UBO.
    LavaCpuBuffer::Config cfg {
        .device = device, .gpu = gpu, .size = sizeof(Uniforms),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };
    mUniforms[0] = LavaCpuBuffer::create(cfg);
    mUniforms[1] = LavaCpuBuffer::create(cfg);

    // Create the sampler.
    VkSamplerCreateInfo samplerInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .minFilter = VK_FILTER_LINEAR,
        .magFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .minLod = 0.0f,
        .maxLod = 0.25f
    };
    vkCreateSampler(device, &samplerInfo, 0, &mSampler);

    // Create the descriptor set.
    mDescriptors = LavaDescCache::create({
        .device = device, .uniformBuffers = { 0 }, .imageSamplers = { {
            .sampler = mSampler,
            .imageView = mOffscreenSurface.color->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        } }
    });
    const VkDescriptorSetLayout dlayout = mDescriptors->getLayout();

    // Create the pipeline.
    static_assert(sizeof(Vertex) == 12, "Unexpected vertex size.");
    mPipelines = LavaPipeCache::create({
        .device = device, .descriptorLayouts = { dlayout }, .renderPass = renderPass,
        .vshader = VK_NULL_HANDLE,
        .fshader = VK_NULL_HANDLE,
        .vertex = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .attributes = { {
                .binding = 0u,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .location = 0u,
                .offset = 0u,
            }, {
                .binding = 0u,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .location = 1u,
                .offset = 8u,
            } },
            .buffers = { {
                .binding = 0u,
                .stride = 12,
            } }
        }
    });
    VkPipelineLayout playout = mPipelines->getLayout();

    // Finish populating the vertex buffer.
    mContext->waitWork();
    delete stage;

    // Fill in some structs that will be used when rendering.
    const VkViewport viewport = {
        .width = (float) extent.width, .height = (float) extent.height
    };
    const VkRect2D scissor { .extent = extent };
    const VkBuffer buffer[] = { mVertexBuffer->getBuffer() };
    const VkDeviceSize offsets[] = { 0 };
    VkRenderPassBeginInfo offscreenRpbi;

    // Record two command buffers.
    mRecording = mContext->createRecording();
    for (uint32_t i = 0; i < 2; i++) {
        const VkCommandBuffer cmdbuffer = mContext->beginRecording(mRecording, i);

        mDescriptors->setUniformBuffer(0, mUniforms[i]->getBuffer());
        mSurfaces->getRenderPass(mOffscreenSurface, &offscreenRpbi);
        mPipelines->setRenderPass(offscreenRpbi.renderPass);
        mPipelines->setVertexShader(mOffscreenProgram->getVertexShader());
        mPipelines->setFragmentShader(mOffscreenProgram->getFragmentShader());

        vkCmdBeginRenderPass(cmdbuffer, &offscreenRpbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelines->getPipeline());
        vkCmdSetViewport(cmdbuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdbuffer, 0, 1, &scissor);
        vkCmdBindVertexBuffers(cmdbuffer, 0, 1, buffer, offsets);
        vkCmdBindDescriptorSets(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1,
                mDescriptors->getDescPointer(), 0, 0);
        vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmdbuffer);

        mPipelines->setRenderPass(renderPass);
        mPipelines->setVertexShader(mBackbufferProgram->getVertexShader());
        mPipelines->setFragmentShader(mBackbufferProgram->getFragmentShader());

        vkCmdBeginRenderPass(cmdbuffer, mContext->getBeginInfo(i), VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelines->getPipeline());
        vkCmdSetViewport(cmdbuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdbuffer, 0, 1, &scissor);
        vkCmdBindVertexBuffers(cmdbuffer, 0, 1, buffer, offsets);
        vkCmdBindDescriptorSets(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1,
                mDescriptors->getDescPointer(), 0, 0);
        vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmdbuffer);

        mContext->endRecording();
    }
}

LavamacApp::~LavamacApp() {
    mContext->waitRecording(mRecording);
    mContext->freeRecording(mRecording);
    mSurfaces->freeAttachment(mOffscreenSurface.color);
    vkDestroySampler(mContext->getDevice(), mSampler, 0);
    delete mSurfaces;
    delete mUniforms[0];
    delete mUniforms[1];
    delete mDescriptors;
    delete mPipelines;
    delete mOffscreenProgram;
    delete mBackbufferProgram;
    delete mVertexBuffer;
    delete mContext;
}

void LavamacApp::draw(double time) {
    Uniforms uniforms {
        .iResolution = {1200, 1200, 0, 0},
        .iTime = (float) time
    };
    mUniforms[0]->setData(&uniforms, sizeof(uniforms));
    mContext->presentRecording(mRecording);
    swap(mUniforms[0], mUniforms[1]);
}

static AmberApplication::Register prefs({
    .title = "lavamac",
    .first = "lavamac",
    .width = 512,
    .height = 512,
    .decorated = false
});

static AmberApplication::Register app("lavamac", [] (AmberApplication::SurfaceFn cb) {
    return new LavamacApp(cb);
});
