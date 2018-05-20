// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaCompiler.h>
#include <par/LavaLog.h>

#include <SPIRV/GlslangToSpv.h>

#include "LavaInternal.h"

using namespace std;
using namespace par;
using namespace spdlog;

class LavaCompilerImpl : public LavaCompiler {
public:
    LavaCompilerImpl() noexcept;
    ~LavaCompilerImpl() noexcept;
    bool compile(Stage stage, string_view glsl, vector<uint32_t>* spirv) const noexcept;
};

LAVA_IMPL_CLASS(LavaCompiler)

extern const TBuiltInResource DefaultTBuiltInResource;

LavaCompiler* LavaCompiler::create() noexcept {
    return new LavaCompilerImpl();
}

void LavaCompiler::destroy(LavaCompiler** that) noexcept {
    delete upcast(*that);
    *that = nullptr;
}

LavaCompilerImpl::LavaCompilerImpl() noexcept {
    glslang::InitializeProcess();
}

LavaCompilerImpl::~LavaCompilerImpl() noexcept {
    glslang::FinalizeProcess();
}

bool LavaCompilerImpl::compile(Stage stage, string_view glsl,
        vector<uint32_t>* spirv) const noexcept {
    EShLanguage lang;
    switch (stage) {
        case VERTEX: lang = EShLangVertex; break;
        case FRAGMENT: lang = EShLangFragment; break;
        case COMPUTE: lang = EShLangCompute; break;
    }
    const EShMessages flags = (EShMessages) (EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);
    glslang::TShader glslShader(lang);
    const char *glslStrings[1] = { glsl.data() };
    glslShader.setStrings(glslStrings, 1);
    const int glslangVersion = 100;
    if (glslShader.parse(&DefaultTBuiltInResource, glslangVersion, false, flags)) {
        if (*glslShader.getInfoLog()) {
            llog.warn(glslShader.getInfoLog());
        }
        if (*glslShader.getInfoDebugLog()) {
            llog.debug(glslShader.getInfoDebugLog());
        }
        glslang::SpvOptions* options = nullptr;
        glslang::GlslangToSpv(*glslShader.getIntermediate(), *spirv, options);
        return true;
    }
    llog.error("Can't compile {}", (stage == EShLangVertex ? "VS" : "FS"));
    llog.warn(glslShader.getInfoLog());
    if (*glslShader.getInfoDebugLog()) {
        llog.debug(glslShader.getInfoDebugLog());
    }
    return false;
}

bool LavaCompiler::compile(Stage stage, string_view glsl, vector<uint32_t>* spirv) const noexcept {
    return upcast(this)->compile(stage, glsl, spirv);
}

const TBuiltInResource DefaultTBuiltInResource = {
        /* .MaxLights = */ 32,
        /* .MaxClipPlanes = */ 6,
        /* .MaxTextureUnits = */ 32,
        /* .MaxTextureCoords = */ 32,
        /* .MaxVertexAttribs = */ 64,
        /* .MaxVertexUniformComponents = */ 4096,
        /* .MaxVaryingFloats = */ 64,
        /* .MaxVertexTextureImageUnits = */ 32,
        /* .MaxCombinedTextureImageUnits = */ 80,
        /* .MaxTextureImageUnits = */ 32,
        /* .MaxFragmentUniformComponents = */ 4096,
        /* .MaxDrawBuffers = */ 32,
        /* .MaxVertexUniformVectors = */ 128,
        /* .MaxVaryingVectors = */ 8,
        /* .MaxFragmentUniformVectors = */ 16,
        /* .MaxVertexOutputVectors = */ 16,
        /* .MaxFragmentInputVectors = */ 15,
        /* .MinProgramTexelOffset = */ -8,
        /* .MaxProgramTexelOffset = */ 7,
        /* .MaxClipDistances = */ 8,
        /* .MaxComputeWorkGroupCountX = */ 65535,
        /* .MaxComputeWorkGroupCountY = */ 65535,
        /* .MaxComputeWorkGroupCountZ = */ 65535,
        /* .MaxComputeWorkGroupSizeX = */ 1024,
        /* .MaxComputeWorkGroupSizeY = */ 1024,
        /* .MaxComputeWorkGroupSizeZ = */ 64,
        /* .MaxComputeUniformComponents = */ 1024,
        /* .MaxComputeTextureImageUnits = */ 16,
        /* .MaxComputeImageUniforms = */ 8,
        /* .MaxComputeAtomicCounters = */ 8,
        /* .MaxComputeAtomicCounterBuffers = */ 1,
        /* .MaxVaryingComponents = */ 60,
        /* .MaxVertexOutputComponents = */ 64,
        /* .MaxGeometryInputComponents = */ 64,
        /* .MaxGeometryOutputComponents = */ 128,
        /* .MaxFragmentInputComponents = */ 128,
        /* .MaxImageUnits = */ 8,
        /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
        /* .MaxCombinedShaderOutputResources = */ 8,
        /* .MaxImageSamples = */ 0,
        /* .MaxVertexImageUniforms = */ 0,
        /* .MaxTessControlImageUniforms = */ 0,
        /* .MaxTessEvaluationImageUniforms = */ 0,
        /* .MaxGeometryImageUniforms = */ 0,
        /* .MaxFragmentImageUniforms = */ 8,
        /* .MaxCombinedImageUniforms = */ 8,
        /* .MaxGeometryTextureImageUnits = */ 16,
        /* .MaxGeometryOutputVertices = */ 256,
        /* .MaxGeometryTotalOutputComponents = */ 1024,
        /* .MaxGeometryUniformComponents = */ 1024,
        /* .MaxGeometryVaryingComponents = */ 64,
        /* .MaxTessControlInputComponents = */ 128,
        /* .MaxTessControlOutputComponents = */ 128,
        /* .MaxTessControlTextureImageUnits = */ 16,
        /* .MaxTessControlUniformComponents = */ 1024,
        /* .MaxTessControlTotalOutputComponents = */ 4096,
        /* .MaxTessEvaluationInputComponents = */ 128,
        /* .MaxTessEvaluationOutputComponents = */ 128,
        /* .MaxTessEvaluationTextureImageUnits = */ 16,
        /* .MaxTessEvaluationUniformComponents = */ 1024,
        /* .MaxTessPatchComponents = */ 120,
        /* .MaxPatchVertices = */ 32,
        /* .MaxTessGenLevel = */ 64,
        /* .MaxViewports = */ 16,
        /* .MaxVertexAtomicCounters = */ 0,
        /* .MaxTessControlAtomicCounters = */ 0,
        /* .MaxTessEvaluationAtomicCounters = */ 0,
        /* .MaxGeometryAtomicCounters = */ 0,
        /* .MaxFragmentAtomicCounters = */ 8,
        /* .MaxCombinedAtomicCounters = */ 8,
        /* .MaxAtomicCounterBindings = */ 1,
        /* .MaxVertexAtomicCounterBuffers = */ 0,
        /* .MaxTessControlAtomicCounterBuffers = */ 0,
        /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
        /* .MaxGeometryAtomicCounterBuffers = */ 0,
        /* .MaxFragmentAtomicCounterBuffers = */ 1,
        /* .MaxCombinedAtomicCounterBuffers = */ 1,
        /* .MaxAtomicCounterBufferSize = */ 16384,
        /* .MaxTransformFeedbackBuffers = */ 4,
        /* .MaxTransformFeedbackInterleavedComponents = */ 64,
        /* .MaxCullDistances = */ 8,
        /* .MaxCombinedClipAndCullDistances = */ 8,
        /* .MaxSamples = */ 4,
        /* .limits = */ {
                /* .nonInductiveForLoops = */ 1,
                /* .whileLoops = */ 1,
                /* .doWhileLoops = */ 1,
                /* .generalUniformIndexing = */ 1,
                /* .generalAttributeMatrixVectorIndexing = */ 1,
                /* .generalVaryingIndexing = */ 1,
                /* .generalSamplerIndexing = */ 1,
                /* .generalVariableIndexing = */ 1,
                /* .generalConstantMatrixVectorIndexing = */ 1,
        }
};
