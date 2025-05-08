#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface FMProcessResult : NSObject

@property(nonatomic, assign) BOOL livenessChecked;
@property(nonatomic, assign) BOOL isLive;
@property(nonatomic, assign) BOOL faceDetected;
@property(nonatomic, assign) BOOL embeddingExtracted;
@property(nonatomic, assign) float livenessScore;
@property(nonatomic, strong) NSArray<NSNumber *> *embedding;

@end

NS_ASSUME_NONNULL_END
