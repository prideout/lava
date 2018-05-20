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
    const EShMessages flags = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules);
    glslang::TShader glslShader(lang);
    const char *glslStrings[] = { glsl.data() };
    glslShader.setStrings(glslStrings, 1);
    const int glslangVersion = 450;
    const bool fwdCompatible = false;
    if (glslShader.parse(&DefaultTBuiltInResource, glslangVersion, fwdCompatible, flags)) {
        if (*glslShader.getInfoLog()) {
            llog.warn(glslShader.getInfoLog());
        }
        if (*glslShader.getInfoDebugLog()) {
            llog.debug(glslShader.getInfoDebugLog());
        }
        glslang::SpvOptions options;
        options.generateDebugInfo = false;
        options.disableOptimizer = false;
        options.optimizeSize = true;
        glslang::GlslangToSpv(*glslShader.getIntermediate(), *spirv, &options);
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

// See ResourceLimits.cpp in glslang/StandAlone
const TBuiltInResource DefaultTBuiltInResource {
    .maxLights = 32,
    .maxClipPlanes = 6,
    .maxTextureUnits = 32,
    .maxTextureCoords = 32,
    .maxVertexAttribs = 64,
    .maxVertexUniformComponents = 4096,
    .maxVaryingFloats = 64,
    .maxVertexTextureImageUnits = 32,
    .maxCombinedTextureImageUnits = 80,
    .maxTextureImageUnits = 32,
    .maxFragmentUniformComponents = 4096,
    .maxDrawBuffers = 32,
    .maxVertexUniformVectors = 128,
    .maxVaryingVectors = 8,
    .maxFragmentUniformVectors = 16,
    .maxVertexOutputVectors = 16,
    .maxFragmentInputVectors = 15,
    .minProgramTexelOffset = -8,
    .maxProgramTexelOffset = 7,
    .maxClipDistances = 8,
    .maxComputeWorkGroupCountX = 65535,
    .maxComputeWorkGroupCountY = 65535,
    .maxComputeWorkGroupCountZ = 65535,
    .maxComputeWorkGroupSizeX = 1024,
    .maxComputeWorkGroupSizeY = 1024,
    .maxComputeWorkGroupSizeZ = 64,
    .maxComputeUniformComponents = 1024,
    .maxComputeTextureImageUnits = 16,
    .maxComputeImageUniforms = 8,
    .maxComputeAtomicCounters = 8,
    .maxComputeAtomicCounterBuffers = 1,
    .maxVaryingComponents = 60,
    .maxVertexOutputComponents = 64,
    .maxGeometryInputComponents = 64,
    .maxGeometryOutputComponents = 128,
    .maxFragmentInputComponents = 128,
    .maxImageUnits = 8,
    .maxCombinedImageUnitsAndFragmentOutputs = 8,
    .maxCombinedShaderOutputResources = 8,
    .maxImageSamples = 0,
    .maxVertexImageUniforms = 0,
    .maxTessControlImageUniforms = 0,
    .maxTessEvaluationImageUniforms = 0,
    .maxGeometryImageUniforms = 0,
    .maxFragmentImageUniforms = 8,
    .maxCombinedImageUniforms = 8,
    .maxGeometryTextureImageUnits = 16,
    .maxGeometryOutputVertices = 256,
    .maxGeometryTotalOutputComponents = 1024,
    .maxGeometryUniformComponents = 1024,
    .maxGeometryVaryingComponents = 64,
    .maxTessControlInputComponents = 128,
    .maxTessControlOutputComponents = 128,
    .maxTessControlTextureImageUnits = 16,
    .maxTessControlUniformComponents = 1024,
    .maxTessControlTotalOutputComponents = 4096,
    .maxTessEvaluationInputComponents = 128,
    .maxTessEvaluationOutputComponents = 128,
    .maxTessEvaluationTextureImageUnits = 16,
    .maxTessEvaluationUniformComponents = 1024,
    .maxTessPatchComponents = 120,
    .maxPatchVertices = 32,
    .maxTessGenLevel = 64,
    .maxViewports = 16,
    .maxVertexAtomicCounters = 0,
    .maxTessControlAtomicCounters = 0,
    .maxTessEvaluationAtomicCounters = 0,
    .maxGeometryAtomicCounters = 0,
    .maxFragmentAtomicCounters = 8,
    .maxCombinedAtomicCounters = 8,
    .maxAtomicCounterBindings = 1,
    .maxVertexAtomicCounterBuffers = 0,
    .maxTessControlAtomicCounterBuffers = 0,
    .maxTessEvaluationAtomicCounterBuffers = 0,
    .maxGeometryAtomicCounterBuffers = 0,
    .maxFragmentAtomicCounterBuffers = 1,
    .maxCombinedAtomicCounterBuffers = 1,
    .maxAtomicCounterBufferSize = 16384,
    .maxTransformFeedbackBuffers = 4,
    .maxTransformFeedbackInterleavedComponents = 64,
    .maxCullDistances = 8,
    .maxCombinedClipAndCullDistances = 8,
    .maxSamples = 4,
    .limits = {
        .nonInductiveForLoops = 1,
        .whileLoops = 1,
        .doWhileLoops = 1,
        .generalUniformIndexing = 1,
        .generalAttributeMatrixVectorIndexing = 1,
        .generalVaryingIndexing = 1,
        .generalSamplerIndexing = 1,
        .generalVariableIndexing = 1,
        .generalConstantMatrixVectorIndexing = 1,
    }
};
