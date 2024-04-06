#include <jni.h>
#include <string>
#include <vector>

#include <android/log.h>

#include "ort_genai_c.h"

namespace {
    void ThrowException(JNIEnv *env, OgaResult *result) {
        __android_log_write(ANDROID_LOG_DEBUG, "native", "ThrowException");
        // copy error so we can release the OgaResult
        jstring jerr_msg = env->NewStringUTF(OgaResultGetError(result));
        OgaDestroyResult(result);

        static const char *className = "ai/onnxruntime/genai/demo/GenAIException";
        jclass exClazz = env->FindClass(className);
        jmethodID exConstructor = env->GetMethodID(exClazz, "<init>", "(Ljava/lang/String;)V");
        jobject javaException = env->NewObject(exClazz, exConstructor, jerr_msg);
        env->Throw(static_cast<jthrowable>(javaException));
    }

    void ThrowIfError(JNIEnv *env, OgaResult *result) {
        if (result != nullptr) {
            ThrowException(env, result);
        }
    }

    // handle conversion/release of jstring to const char*
    struct CString {
        CString(JNIEnv *env, jstring str)
                : env_{env}, str_{str}, cstr{env->GetStringUTFChars(str, /* isCopy */ nullptr)} {
        }

        const char *cstr;

        operator const char *() const { return cstr; }

        ~CString() {
            env_->ReleaseStringUTFChars(str_, cstr);
        }

    private:
        JNIEnv *env_;
        jstring str_;
    };
}

extern "C" JNIEXPORT jlong JNICALL
Java_ai_onnxruntime_genai_demo_GenAIWrapper_loadModel(JNIEnv *env, jobject thiz, jstring model_path) {
    CString path{env, model_path};
    __android_log_print(ANDROID_LOG_DEBUG, "native", "loadModel %s", path.cstr);

    OgaModel *model = nullptr;
    OgaResult *result = OgaCreateModel(path, &model);
    __android_log_print(ANDROID_LOG_DEBUG, "native", "model address %p", model);

    ThrowIfError(env, result);

    return (jlong)model;
}

extern "C" JNIEXPORT void JNICALL
Java_ai_onnxruntime_genai_demo_GenAIWrapper_releaseModel(JNIEnv *env, jobject thiz, jlong native_model) {
    auto* model = reinterpret_cast<OgaModel*>(native_model);
    __android_log_print(ANDROID_LOG_DEBUG, "native", "releaseModel: %p", model);
    OgaDestroyModel(model);
}

extern "C" JNIEXPORT jlong JNICALL
Java_ai_onnxruntime_genai_demo_GenAIWrapper_createTokenizer(JNIEnv *env, jobject thiz, jlong native_model) {
    const auto* model = reinterpret_cast<const OgaModel*>(native_model);
    OgaTokenizer *tokenizer = nullptr;
    OgaResult* result = OgaCreateTokenizer(model, &tokenizer);
    __android_log_print(ANDROID_LOG_DEBUG, "native", "tokenizer address: %p", tokenizer);

    ThrowIfError(env, result);

    return (jlong)tokenizer;
}

extern "C" JNIEXPORT void JNICALL
Java_ai_onnxruntime_genai_demo_GenAIWrapper_releaseTokenizer(JNIEnv *env, jobject thiz, jlong native_tokenizer) {
    auto* tokenizer = reinterpret_cast<OgaTokenizer*>(native_tokenizer);
    __android_log_print(ANDROID_LOG_DEBUG, "native", "releaseTokenizer: %p", tokenizer);
    OgaDestroyTokenizer(tokenizer);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_ai_onnxruntime_genai_demo_GenAIWrapper_run(JNIEnv *env, jobject thiz, jlong native_model, jlong native_tokenizer,
                                                jstring jprompt, jboolean use_callback) {
    using SequencesPtr = std::unique_ptr<OgaSequences, std::function<void(OgaSequences*)>>;
    using GeneratorParamsPtr = std::unique_ptr<OgaGeneratorParams, std::function<void(OgaGeneratorParams*)>>;
    using TokenizerStreamPtr = std::unique_ptr<OgaTokenizerStream, std::function<void(OgaTokenizerStream*)>>;
    using GeneratorPtr = std::unique_ptr<OgaGenerator, std::function<void(OgaGenerator*)>>;

    auto* model = reinterpret_cast<OgaModel*>(native_model);
    auto* tokenizer = reinterpret_cast<OgaTokenizer*>(native_tokenizer);

    CString prompt{env, jprompt};

    const auto check_result = [env](OgaResult* result) {
        ThrowIfError(env, result);
    };

    // var sequences = tokenizer.Encode(prompt);
    OgaSequences* sequences = nullptr;
    check_result(OgaCreateSequences(&sequences));
    SequencesPtr seq_cleanup{sequences, OgaDestroySequences};

    check_result(OgaTokenizerEncode(tokenizer, prompt, sequences));

    // using GeneratorParams generatorParams = new GeneratorParams(model);
    OgaGeneratorParams* generator_params = nullptr;
    check_result(OgaCreateGeneratorParams(model, &generator_params));
    GeneratorParamsPtr gp_cleanup{generator_params, OgaDestroyGeneratorParams};

    // generatorParams.SetSearchOption("max_length", 200);
    check_result(OgaGeneratorParamsSetSearchNumber(generator_params, "max_length", 80));  // TODO: Rename this API. 'search number' is really opaque
    // generatorParams.SetInputSequences(sequences);
    check_result(OgaGeneratorParamsSetInputSequences(generator_params, sequences));

    __android_log_print(ANDROID_LOG_DEBUG, "native", "starting token generation");

    const auto decode_tokens = [&](const int32_t* tokens, size_t num_tokens){
        const char* output_text = nullptr;
        check_result(OgaTokenizerDecode(tokenizer, tokens, num_tokens, &output_text));
        jstring text = env->NewStringUTF(output_text);
        OgaDestroyString(output_text);
        return text;
    };

    jstring output_text;

    if (!use_callback) {
        // var outputSequences = model.Generate(generatorParams);
        OgaSequences *output_sequences = nullptr;
        check_result(OgaGenerate(model, generator_params, &output_sequences));
        SequencesPtr output_seq_cleanup(output_sequences, OgaDestroySequences);

        size_t num_sequences = OgaSequencesCount(output_sequences);
        __android_log_print(ANDROID_LOG_DEBUG, "native", "%zu sequences generated", num_sequences);

        // var outputString = tokenizer.Decode(outputSequences[0]);
        // TODO: Is there only ever 1 sequence in the output? Handling just one for simplicity for now.
        const int32_t* tokens = OgaSequencesGetSequenceData(output_sequences, 0);
        size_t num_tokens = OgaSequencesGetSequenceCount(output_sequences, 0);

        output_text = decode_tokens(tokens, num_tokens);
    }
    else {
        // using var tokenizerStream = tokenizer.CreateStream();
        OgaTokenizerStream* tokenizer_stream = nullptr;
        check_result(OgaCreateTokenizerStream(tokenizer, &tokenizer_stream));
        TokenizerStreamPtr stream_cleanup(tokenizer_stream, OgaDestroyTokenizerStream);

        // using var generator = new Generator(model, generatorParams);
        OgaGenerator *generator = nullptr;
        check_result(OgaCreateGenerator(model, generator_params, &generator));
        GeneratorPtr gen_cleanup(generator, OgaDestroyGenerator);

        // setup the callback to GenAIWrapper::gotNextToken
        jclass genai_wrapper = env->GetObjectClass(thiz);
        jmethodID callback_id = env->GetMethodID(genai_wrapper, "gotNextToken", "(Ljava/lang/String;)V");
        const auto do_callback = [&](const char* token){
            jstring jtoken = env->NewStringUTF(token);
            env->CallVoidMethod(thiz, callback_id, jtoken);
            env->DeleteLocalRef(jtoken);
        };

        // while (!generator.IsDone())
        while (!OgaGenerator_IsDone(generator)) {
            // generator.ComputeLogits();
            // generator.GenerateNextTokenTop();
            check_result(OgaGenerator_ComputeLogits(generator));
            check_result(OgaGenerator_GenerateNextToken_Top(generator));

            // TODO: Do we need to do something to ensure there's only one sequence being generated?
            // TODO: seem to lack a way to get the number of sequences in the generator as there's no equivalent to
            //       OgaSequencesCount
            const int32_t* seq = OgaGenerator_GetSequence(generator, 0);
            size_t seq_len = OgaGenerator_GetSequenceLength(generator, 0);  // last token
            const char* token = nullptr;
            check_result(OgaTokenizerStreamDecode(tokenizer_stream, seq[seq_len - 1], &token));
            do_callback(token);
            // Destroy is (assumably) not required for OgaTokenizerStreamDecode based on this which seems to indicate
            // the tokenizer is re-using memory for each call.
            //  `'out' is valid until the next call to OgaTokenizerStreamDecode
            //   or when the OgaTokenizerStream is destroyed`
            // OgaDestroyString(token); This causes 'Scudo ERROR: misaligned pointer when deallocating address'
        }

        // decode overall
        const int32_t* tokens = OgaGenerator_GetSequence(generator, 0);
        size_t num_tokens = OgaGenerator_GetSequenceLength(generator, 0);
        output_text = decode_tokens(tokens, num_tokens);
    }

    return output_text;
}
