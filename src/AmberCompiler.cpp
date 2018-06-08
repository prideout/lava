// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/AmberCompiler.h>
#include <par/LavaLog.h>

#include <SPIRV/GlslangToSpv.h>

#include "LavaInternal.h"

using namespace std;
using namespace par;
using namespace spdlog;

static int ninstances = 0;

struct AmberCompilerImpl : AmberCompiler {
    AmberCompilerImpl() noexcept;
    ~AmberCompilerImpl() noexcept;
    bool compile(Stage stage, const string& glsl, vector<uint32_t>* spirv) const noexcept;
};

LAVA_DEFINE_UPCAST(AmberCompiler)

extern const TBuiltInResource DefaultTBuiltInResource;

AmberCompiler* AmberCompiler::create() noexcept {
    return new AmberCompilerImpl();
}

void AmberCompiler::operator delete(void* ptr) noexcept {
    auto impl = (AmberCompilerImpl*) ptr;
    ::delete impl;
}

AmberCompilerImpl::~AmberCompilerImpl() noexcept {
    if (--ninstances == 0) {
        glslang::FinalizeProcess();
    }
}

AmberCompilerImpl::AmberCompilerImpl() noexcept {
    if (ninstances++ == 0) {
        glslang::InitializeProcess();
    }
}

bool AmberCompilerImpl::compile(Stage stage, const string& glsl,
        vector<uint32_t>* spirv) const noexcept {
    // Create the glslang shader object.
    EShLanguage lang;
    switch (stage) {
        case VERTEX: lang = EShLangVertex; break;
        case FRAGMENT: lang = EShLangFragment; break;
        case COMPUTE: lang = EShLangCompute; break;
    }
    glslang::TShader shader(lang);
    const char *glslStrings[] = { glsl.data() };
    shader.setStrings(glslStrings, 1);

    // Compile the shader program.
    const EShMessages flags = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules);
    const int glslangVersion = 100;
    const bool fwdCompatible = false;
    if (!shader.parse(&DefaultTBuiltInResource, glslangVersion, fwdCompatible, flags)) {
        llog.error("Can't compile {}", (stage == EShLangVertex ? "VS" : "FS"));
        llog.warn(shader.getInfoLog());
        if (*shader.getInfoDebugLog()) {
            llog.debug(shader.getInfoDebugLog());
        }
        return false;
    }
    if (*shader.getInfoLog()) {
        llog.warn(shader.getInfoLog());
    }
    if (*shader.getInfoDebugLog()) {
        llog.debug(shader.getInfoDebugLog());
    }

    // Link a shader program containing the single shader.
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(flags)) {
        llog.error("Can't link {}", (stage == EShLangVertex ? "VS" : "FS"));
        if (program.getInfoLog()) {
            llog.warn(program.getInfoLog());
        }
        if (program.getInfoDebugLog()) {
            llog.debug(program.getInfoDebugLog());
        }
        return false;
    }

    // Output the SPIR-V code from the shader program
    glslang::GlslangToSpv(*program.getIntermediate(lang), *spirv);

    return true;
}

bool AmberCompiler::compile(Stage stage, const string& glsl,
        vector<uint32_t>* spirv) const noexcept {
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
