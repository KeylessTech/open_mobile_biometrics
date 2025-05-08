#import <Foundation/Foundation.h>
#import "FMProcessResult.h"

NS_ASSUME_NONNULL_BEGIN

@interface FaceMatchBridge : NSObject

+ (instancetype)sharedInstance;

// Initializes FMCore with JSON config and optional base path
- (BOOL)initWithConfig:(NSString *)configJson basePath:(NSString *)basePath;

// Runs the full processing pipeline on the image
- (FMProcessResult *)processImageAtPath:(NSString *)imagePath skipLiveness:(BOOL)skipLiveness;

// Computes similarity between two embeddings
- (BOOL)matchEmbedding:(NSArray<NSNumber *> *)embedding1
         withEmbedding:(NSArray<NSNumber *> *)embedding2;

// Resets the internal engine state
- (void)reset;


- (void)setDebugSavePath:(NSString*)path;

@end


NS_ASSUME_NONNULL_END
