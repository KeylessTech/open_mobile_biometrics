#import "FaceMatchBridge.h"
#import "FMCore.h"

extern void setDebugSavePath(const std::string&);


@implementation FaceMatchBridge {
    FMCore engine;
}

+ (instancetype)sharedInstance {
    static FaceMatchBridge *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[FaceMatchBridge alloc] init];
    });
    return instance;
}

- (BOOL)initWithConfig:(NSString *)configJson basePath:(NSString *)basePath {
    std::string configStr = [configJson UTF8String];
    std::string basePathStr = [basePath UTF8String];
    return engine.init(configStr, basePathStr);
}


- (FMProcessResult *)processImageAtPath:(NSString *)imagePath skipLiveness:(BOOL)skipLiveness {
    std::string pathStr = [imagePath UTF8String];
    PipelineMode mode = skipLiveness ? PipelineMode::SkipLiveness : PipelineMode::WholePipeline;

    ProcessResult result = engine.process(pathStr, mode);

    FMProcessResult *wrapped = [[FMProcessResult alloc] init];
    wrapped.livenessChecked = result.livenessChecked;
    wrapped.isLive = result.isLive;
    wrapped.faceDetected = result.faceDetected;
    wrapped.embeddingExtracted = result.embeddingExtracted;
    wrapped.livenessScore = result.livenessScore;

    NSMutableArray<NSNumber *> *embeddingArray = [NSMutableArray arrayWithCapacity:result.embedding.size()];
    for (float val : result.embedding) {
        [embeddingArray addObject:@(val)];
    }
    wrapped.embedding = embeddingArray;

    return wrapped;
}


- (BOOL)matchEmbedding:(NSArray<NSNumber *> *)embedding1
         withEmbedding:(NSArray<NSNumber *> *)embedding2 {

    std::vector<float> vec1, vec2;

    for (NSNumber *n in embedding1) {
        vec1.push_back(n.floatValue);
    }
    for (NSNumber *n in embedding2) {
        vec2.push_back(n.floatValue);
    }

    return engine.match(vec1, vec2);
}

- (void)reset {
    engine.reset();
}

// Pass through to C++ side
- (void)setDebugSavePath:(NSString*)path {
  std::string p = [path UTF8String];
  setDebugSavePath(p);
}

@end
