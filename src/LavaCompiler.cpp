// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaCompiler.h>
#include <spdlog/spdlog.h>
#include <SPIRV/GlslangToSpv.h>

using namespace par;
using namespace spdlog;

class LavaCompilerImpl : public LavaCompiler {
public:
    LavaCompilerImpl();
    ~LavaCompilerImpl();
    bool compile(Stage stage, std::string_view glsl,
        std::vector<uint32_t>* spirv) const;
    std::shared_ptr<spdlog::logger> mConsole = spdlog::stdout_color_mt("console");
};

// BEGIN BoilerplateImpl
inline LavaCompilerImpl& upcast(LavaCompiler& that) noexcept {
    return static_cast<LavaCompilerImpl &>(that);
}
inline const LavaCompilerImpl& upcast(const LavaCompiler& that) noexcept {
    return static_cast<const LavaCompilerImpl &>(that);
}
inline LavaCompilerImpl* upcast(LavaCompiler* that) noexcept {
    return static_cast<LavaCompilerImpl *>(that);
}
inline LavaCompilerImpl const* upcast(LavaCompiler const* that) noexcept {
    return static_cast<LavaCompilerImpl const *>(that);
}
LavaCompiler* LavaCompiler::create() {
    return new LavaCompilerImpl();
}
void LavaCompiler::destroy(LavaCompiler** that) {
    delete upcast(*that);
    *that = nullptr;
}
// END BoilerplateImpl

extern const TBuiltInResource DefaultTBuiltInResource;

LavaCompilerImpl::LavaCompilerImpl() {
    glslang::InitializeProcess();
    mConsole->set_pattern("%T %t %^%v%$");
}

LavaCompilerImpl::~LavaCompilerImpl() {
    glslang::FinalizeProcess();
}

bool LavaCompilerImpl::compile(Stage stage, std::string_view glsl,
    std::vector<uint32_t>* spirv) const {
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
            mConsole->warn("{}", glslShader.getInfoLog());
        }
        if (*glslShader.getInfoDebugLog()) {
            mConsole->debug("{}", glslShader.getInfoDebugLog());
        }
        glslang::SpvOptions* options = nullptr;
        glslang::GlslangToSpv(*glslShader.getIntermediate(), *spirv, options);
        return true;
    }
    mConsole->error("Can't compile {}", (stage == EShLangVertex ? "VS" : "FS"));
    mConsole->warn("{}", glslShader.getInfoLog());
    if (*glslShader.getInfoDebugLog()) {
        mConsole->debug("{}", glslShader.getInfoDebugLog());
    }
    return false;
}

bool LavaCompiler::compile(Stage stage,
    std::string_view glsl,
    std::vector<uint32_t>* spirv) const {
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
