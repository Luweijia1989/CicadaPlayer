//
// Created by pingkai on 2020/11/27.
//

#import "SampleDisplayLayerRender.h"
#include "DisplayLayerImpl-interface.h"
#include <base/media/PBAFFrame.h>
#define LOG_TAG "DisplayLayerImpl"
#import <render/video/glRender/base/utils.h>
#include <utility>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#elif TARGET_OS_OSX
#import <AppKit/AppKit.h>
#endif
#include <utils/frame_work_log.h>

@implementation SampleDisplayLayerRender {
    CALayer *parentLayer;
    AVLayerVideoGravity videoGravity;
    CVPixelBufferRef renderingBuffer;
    bool removed;
}
DisplayLayerImpl::DisplayLayerImpl()
{}
DisplayLayerImpl::~DisplayLayerImpl()
{
    if (renderHandle) {
        [(__bridge id) renderHandle removeObserver];
        CFRelease(renderHandle);
    }
}
void DisplayLayerImpl::init()
{
    SampleDisplayLayerRender *object = [[SampleDisplayLayerRender alloc] init];
    renderHandle = (__bridge_retained void *) object;
}
int DisplayLayerImpl::createLayer()
{
    return [(__bridge id) renderHandle createLayer];
}
void DisplayLayerImpl::applyRotate()
{
    int rotate = static_cast<int>(llabs((mFrameRotate + mRotate) % 360));
    [(__bridge id) renderHandle setRotateMode:rotate];
}

int DisplayLayerImpl::renderFrame(IAFFrame *frame)
{
    auto *pbafFrame = dynamic_cast<PBAFFrame *>(frame);

    if (frame->getInfo().video.rotate != mFrameRotate) {
        mFrameRotate = frame->getInfo().video.rotate;
        applyRotate();
    }
    if (frame->getInfo().video.height != mFrameHeight || frame->getInfo().video.width != mFrameWidth ||
        frame->getInfo().video.dar != mFrameDar) {

        mFrameHeight = frame->getInfo().video.height;
        mFrameWidth = frame->getInfo().video.width;
        mFrameDar = frame->getInfo().video.dar;


        if (mFrameDar) {
            mFrameDisplayWidth = static_cast<int>(frame->getInfo().video.dar * frame->getInfo().video.height);
        } else {
            mFrameDisplayWidth = frame->getInfo().video.width;
        }
        mFrameDisplayHeight = frame->getInfo().video.height;
        CGSize size;
        size.width = mFrameDisplayWidth;
        size.height = mFrameDisplayHeight;
        [(__bridge id) renderHandle setVideoSize:size];
    }

    if (pbafFrame) {
        [(__bridge id) renderHandle displayPixelBuffer:pbafFrame->getPixelBuffer()];
    }

    return 0;
}
void DisplayLayerImpl::setDisplay(void *display)
{
    return [(__bridge id) renderHandle setDisplay:display];
}
void DisplayLayerImpl::clearScreen()
{
    [(__bridge id) renderHandle clearScreen];
}

void DisplayLayerImpl::setScale(IVideoRender::Scale scale)
{
    [(__bridge id) renderHandle setVideoScale:scale];
}

void DisplayLayerImpl::setFilpByRotate()
{
    int rotate = static_cast<int>(llabs((mFrameRotate + mRotate) % 180));
    switch (mFlip) {
        case IVideoRender::Flip_None:
            [(__bridge id) renderHandle setMirrorTransform:CATransform3DMakeRotation(0, 0, 0, 0)];
            break;
        case IVideoRender::Flip_Horizontal:
            if (rotate == 0) {
                [(__bridge id) renderHandle setMirrorTransform:CATransform3DMakeRotation(M_PI, 0, 1, 0)];
            } else {
                [(__bridge id) renderHandle setMirrorTransform:CATransform3DMakeRotation(M_PI, 1, 0, 0)];
            }
            break;
        case IVideoRender::Flip_Vertical:
            if (rotate == 0) {
                [(__bridge id) renderHandle setMirrorTransform:CATransform3DMakeRotation(M_PI, 1, 0, 0)];
            } else {
                [(__bridge id) renderHandle setMirrorTransform:CATransform3DMakeRotation(M_PI, 0, 1, 0)];
            }
            break;
        default:
            break;
    }
}

void DisplayLayerImpl::setFlip(IVideoRender::Flip flip)
{
    if (mFlip != flip) {
        mFlip = flip;
        setFilpByRotate();
        [(__bridge id) renderHandle applyTransform];
    }
}

void DisplayLayerImpl::setBackgroundColor(uint32_t color)
{
    float fColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    cicada::convertToGLColor(color, fColor);
    CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
    CGColorRef cgColor = CGColorCreate(rgb, (CGFloat[]){fColor[0], fColor[1], fColor[2], fColor[3]});
    [(__bridge id) renderHandle setBGColour:cgColor];
    CGColorRelease(cgColor);
    CGColorSpaceRelease(rgb);
}

void DisplayLayerImpl::captureScreen(std::function<void(uint8_t *, int, int)> func)
{
    void *img = [(__bridge id) renderHandle captureScreen];
    func((uint8_t *) img, -1, -1);
}
void DisplayLayerImpl::reDraw()
{
    [(__bridge id) renderHandle reDraw];
}

void DisplayLayerImpl::setRotate(IVideoRender::Rotate rotate)
{
    if (mRotate != rotate) {
        mRotate = rotate;
        setFilpByRotate();
        applyRotate();
    }
}

- (instancetype)init
{
    videoGravity = AVLayerVideoGravityResizeAspect;
    parentLayer = nil;
    _bGColour = nullptr;
    _mirrorTransform = CATransform3DMakeRotation(0, 0, 0, 0);
    _rotateTransform = CATransform3DMakeRotation(0, 0, 0, 0);
    _scaleTransform = CATransform3DMakeRotation(0, 0, 0, 0);
    renderingBuffer = nullptr;
    removed = false;
    return self;
}

- (CATransform3D)applyDARTransform:(CATransform3D)transform
{
    assert(self.displayLayer.videoGravity != AVLayerVideoGravityResizeAspectFill);
    if (self.displayLayer.videoGravity == AVLayerVideoGravityResize) {
        float scalex = 1;
        float scaley = 1;

        CGSize displaySize = [self getVideoSize];
        float videoDar = displaySize.width / displaySize.height;

        float scaleW = static_cast<float>(self.displayLayer.bounds.size.width / self.displayLayer.bounds.size.height) / videoDar;
        bool bFillWidth = self.displayLayer.bounds.size.width / self.displayLayer.bounds.size.height < _frameSize.width / _frameSize.height;
        if (bFillWidth) {
            scaley = scaleW;
        } else {
            scalex = 1 / scaleW;
        }

        CATransform3D dar;
        if (_rotateMode % 180 == 0) {
            dar = CATransform3DMakeScale(scalex, scaley, 1);
        } else
            dar = CATransform3DMakeScale(scaley, scalex, 1);
        return CATransform3DConcat(dar, transform);
    }
    return transform;
}
- (void)initLayer
{
    if (removed) {
        AF_LOGW("initLayer removed\n");
        return;
    }
    if (!self.displayLayer) {
        self.displayLayer = [AVSampleBufferDisplayLayer layer];
        self.displayLayer.videoGravity = AVLayerVideoGravityResize;
    }
    [parentLayer addSublayer:self.displayLayer];
    if (_bGColour) {
        parentLayer.backgroundColor = _bGColour;
    }
    parentLayer.masksToBounds = YES;
    self.displayLayer.frame = parentLayer.bounds;
    self.displayLayer.transform = [self CalculateTransform];
    [parentLayer addObserver:self forKeyPath:@"bounds" options:NSKeyValueObservingOptionNew context:nil];
}

- (CATransform3D)CalculateTransform
{
    CGSize disPlaySize = [self getVideoSize];
    CATransform3D transform = CATransform3DConcat(_mirrorTransform, _rotateTransform);
    if (_rotateMode % 180) {
        std::swap(disPlaySize.width, disPlaySize.height);
    }

    bool bFillWidth = self.displayLayer.bounds.size.width / self.displayLayer.bounds.size.height < disPlaySize.width / disPlaySize.height;

    float scalex = 1.0;
    float scaley = 1.0;

    if (videoGravity == AVLayerVideoGravityResizeAspect) {
        if (bFillWidth) {
            scalex = static_cast<float>(self.displayLayer.bounds.size.width / disPlaySize.width);
        } else {
            scalex = static_cast<float>(self.displayLayer.bounds.size.height / disPlaySize.height);
        }
        scaley = scalex;
    } else if (videoGravity == AVLayerVideoGravityResizeAspectFill) {
        if (!bFillWidth) {
            scalex = static_cast<float>(self.displayLayer.bounds.size.width / disPlaySize.width);
        } else {
            scalex = static_cast<float>(self.displayLayer.bounds.size.height / disPlaySize.height);
        }
        scaley = scalex;
    } else if (videoGravity == AVLayerVideoGravityResize) {
        scalex = static_cast<float>(self.displayLayer.bounds.size.width / disPlaySize.width);
        scaley = static_cast<float>(self.displayLayer.bounds.size.height / disPlaySize.height);
    }
    _scaleTransform = CATransform3DMakeScale(scalex, scaley, 1);
    _scaleTransform = [self applyDARTransform:_scaleTransform];
    return CATransform3DConcat(transform, _scaleTransform);
}

- (void)applyTransform
{
    if (self.displayLayer) {
        dispatch_async(dispatch_get_main_queue(), ^{
          self.displayLayer.transform = [self CalculateTransform];
          [self.displayLayer removeAllAnimations];
        });
    }
}

- (void)setDisplay:(void *)layer
{
    if (layer != (__bridge void *) parentLayer) {
        parentLayer = (__bridge CALayer *) layer;
        if (strcmp(dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL), dispatch_queue_get_label(dispatch_get_main_queue())) == 0) {
            [self initLayer];
        } else {
            dispatch_async(dispatch_get_main_queue(), ^{
              [self initLayer];
            });
        }
    }
}

- (void)setBGColour:(CGColorRef)bGColour
{
    assert(bGColour);
    CGColorRef color = CGColorRetain(bGColour);
    dispatch_async(dispatch_get_main_queue(), ^{
      if (_bGColour) {
          CGColorRelease(_bGColour);
      }
      _bGColour = color;
      if (parentLayer) {
          parentLayer.backgroundColor = _bGColour;
      }
    });
}

- (void)setVideoScale:(IVideoRender::Scale)scale
{
    switch (scale) {
        case IVideoRender::Scale_AspectFit:
            videoGravity = AVLayerVideoGravityResizeAspect;
            break;
        case IVideoRender::Scale_AspectFill:
            videoGravity = AVLayerVideoGravityResizeAspectFill;
            break;
        case IVideoRender::Scale_Fill:
            videoGravity = AVLayerVideoGravityResize;
            break;
        default:
            return;
    }
    [self applyTransform];
}

- (void)setRotateMode:(int)rotateMode
{
    _rotateMode = rotateMode;
    switch (rotateMode) {
        case 0:
            _rotateTransform = CATransform3DMakeRotation(0, 0, 0, 0);
            break;
        case 90:
            _rotateTransform = CATransform3DMakeRotation(M_PI_2, 0, 0, 1);
            break;
        case 180:
            _rotateTransform = CATransform3DMakeRotation(M_PI, 0, 0, 1);
            break;
        case 270:
            _rotateTransform = CATransform3DMakeRotation(M_PI_2, 0, 0, -1);
            break;

        default:
            break;
    }
    [self applyTransform];
}
#if TARGET_OS_IPHONE
- (void *)captureScreen
{
    CGSize newSize = CGSizeMake(10, 10);
    UIImage *uiImage = nil;
    CGFloat scale = [UIScreen mainScreen].scale;
    if (self.displayLayer) {
        newSize = CGSizeMake(self.displayLayer.bounds.size.width * scale, self.displayLayer.bounds.size.height * scale);
    }

    CGSize disPlaySize = [self getVideoSize];
    if (self.displayLayer.videoGravity == AVLayerVideoGravityResize) {
        // function imageTransform() will apply all transforms we applied to displayLayer,
        // so here we use the displayLayer's origin size
        disPlaySize = self.displayLayer.bounds.size;
    }

    if (renderingBuffer && self.displayLayer) {
        CGSize imageSize = CGSizeMake(disPlaySize.width * scale, disPlaySize.height * scale);
        CIImage *ciImage = [CIImage imageWithCVPixelBuffer:renderingBuffer];
        uiImage = [UIImage imageWithCIImage:ciImage];
        UIGraphicsBeginImageContext(imageSize);

        [uiImage drawInRect:CGRectMake(0, 0, imageSize.width, imageSize.height)];
        uiImage = UIGraphicsGetImageFromCurrentImageContext();
        UIGraphicsEndImageContext();
    }

    uiImage = [self imageTransform:uiImage inSize:newSize];

    return (void *) CFBridgingRetain(uiImage);
}

- (UIImage *)imageTransform:(UIImage*)img inSize:(CGSize)newSize
{
    UIGraphicsBeginImageContext(newSize);
    CGContextRef ctx = UIGraphicsGetCurrentContext();
    CGImageRef cgImage = img.CGImage;
    
    if (!_bGColour) {
        _bGColour = [UIColor blackColor].CGColor;
    }

    //draw the background color
    CGRect bgRect = CGRectMake(0.0f, 0.0f, newSize.width, newSize.height);
    CGContextSetFillColorWithColor(ctx, _bGColour);
    CGContextSetBlendMode(ctx, kCGBlendModeNormal);
    CGContextFillRect(ctx, bgRect);

    if (ctx == NULL || cgImage == NULL) {
        UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
        UIGraphicsEndImageContext();
        return image ?: img;
    }

    CGContextTranslateCTM(ctx, newSize.width / 2.0, newSize.height / 2.0);
    CGContextConcatCTM(ctx, CATransform3DGetAffineTransform(self.displayLayer.transform));
    CGContextScaleCTM(ctx, 1, -1);
    CGPoint origin = CGPointMake(-(img.size.width / 2.0), -(img.size.height / 2.0));
    CGRect rect = CGRectZero;
    rect.origin = origin;
    rect.size = img.size;
    CGContextDrawImage(ctx, rect, cgImage);

    UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return image ?: img;
}
#elif TARGET_OS_OSX
- (NSImage *)imageFromPixelBuffer:(CVPixelBufferRef)pixelBufferRef
{
    CGSize newSize = CGSizeMake(self.displayLayer.bounds.size.width, self.displayLayer.bounds.size.height);
    
    CVImageBufferRef imageBuffer = pixelBufferRef;

    //    CVPixelBufferLockBaseAddress(imageBuffer, 0);
    //    void *baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
    size_t width = CVPixelBufferGetWidth(imageBuffer);
    size_t height = CVPixelBufferGetHeight(imageBuffer);
    CIImage *coreImage = [CIImage imageWithCVPixelBuffer:pixelBufferRef];
    CIContext *temporaryContext = [CIContext contextWithOptions:nil];
    
    CGSize disPlaySize = [self getVideoSize];
//    if (self.displayLayer.videoGravity == AVLayerVideoGravityResize) {
//        // function imageTransform() will apply all transforms we applied to displayLayer,
//        // so here we use the displayLayer's origin size
//        disPlaySize = self.displayLayer.bounds.size;
//    }

    CGImageRef videoImage = [temporaryContext createCGImage:coreImage fromRect:CGRectMake(0, 0,width,height)];
    NSImage *image = [[NSImage alloc] initWithSize:CGSizeMake(newSize.width, newSize.height)];
    //    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
    // TODO: imageTransform
    NSBitmapImageRep* rep = [[NSBitmapImageRep alloc]
                                initWithBitmapDataPlanes:NULL
                                              pixelsWide:newSize.width
                                              pixelsHigh:newSize.height
                                           bitsPerSample:8
                                         samplesPerPixel:4
                                                hasAlpha:YES
                                                isPlanar:NO
                                          colorSpaceName:NSCalibratedRGBColorSpace
                                             bytesPerRow:0
                                            bitsPerPixel:0];

    [image addRepresentation:rep];

    [image lockFocus];

    if (!_bGColour) {
        _bGColour = [NSColor blackColor].CGColor;
    }
    
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    CGContextClearRect(ctx, NSMakeRect(0, 0, newSize.width, newSize.height));
    CGContextSetFillColorWithColor(ctx,_bGColour);
    CGContextFillRect(ctx, NSMakeRect(0, 0, newSize.width, newSize.height));
    
    CGContextTranslateCTM(ctx, newSize.width / 2.0, newSize.height / 2.0);
    CGContextConcatCTM(ctx, CATransform3DGetAffineTransform(self.displayLayer.transform));
//    CGContextScaleCTM(ctx, 1, -1);
    CGPoint origin = CGPointMake(-(newSize.width / 2.0), -(newSize.height / 2.0));
    CGRect rect = CGRectZero;
    rect.origin = origin;
    rect.size = image.size;
    CGContextDrawImage(ctx, rect, videoImage);

    [image unlockFocus];
    
    return image;
}


- (void *)captureScreen
{
    if (renderingBuffer && self.displayLayer) {
        NSImage *image = [self imageFromPixelBuffer:renderingBuffer];
        return (void *) CFBridgingRetain(image);
    }
    return nullptr;
}

#endif

- (void)clearScreen
{
    [self.displayLayer flushAndRemoveImage];
    if (renderingBuffer) {
        CVPixelBufferRelease(renderingBuffer);
    }
    renderingBuffer = nullptr;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSString *, id> *)change context:(void *)context
{
    if ([keyPath isEqualToString:@"bounds"]) {
#if TARGET_OS_IPHONE
        CGRect bounds = [change[NSKeyValueChangeNewKey] CGRectValue];
#elif TARGET_OS_OSX
        NSRect bounds = [change[@"new"] rectValue];
#endif
        self.displayLayer.frame = CGRectMake(0, 0, bounds.size.width, bounds.size.height);
        self.displayLayer.bounds = CGRectMake(0, 0, bounds.size.width, bounds.size.height);
        [self getVideoSize];
        [self applyTransform];
    }
}

- (int)createLayer
{
    //  NSLog(@"createLayer");
    return 0;
};

- (void)setVideoSize:(CGSize)videoSize
{
    _frameSize = videoSize;
    [self getVideoSize];
    [self applyTransform];
}

- (CGSize)getVideoSize
{
    float scale = 1;
    bool bFillWidth = self.displayLayer.bounds.size.width / self.displayLayer.bounds.size.height < _frameSize.width / _frameSize.height;
    if (bFillWidth) {
        scale = static_cast<float>(self.displayLayer.bounds.size.width / _frameSize.width);
    } else {
        scale = static_cast<float>(self.displayLayer.bounds.size.height / _frameSize.height);
    }
    return CGSizeMake(_frameSize.width * scale, _frameSize.height * scale);
}

- (void)reDraw
{
    [self displayPixelBuffer:renderingBuffer];
}

- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer
{
    if (!pixelBuffer || !self.displayLayer) {
        return;
    }
    
    if(!self.displayLayer.superlayer.superlayer.superlayer){
        return;
    }

    if (pixelBuffer != renderingBuffer) {
        if (renderingBuffer) {
            CVPixelBufferRelease(renderingBuffer);
        }
        renderingBuffer = CVPixelBufferRetain(pixelBuffer);
    }

    CMSampleTimingInfo timing = {kCMTimeInvalid, kCMTimeInvalid, kCMTimeInvalid};

    CMVideoFormatDescriptionRef videoInfo = nullptr;
    OSStatus result = CMVideoFormatDescriptionCreateForImageBuffer(nullptr, pixelBuffer, &videoInfo);
    NSParameterAssert(result == 0 && videoInfo != nullptr);

    CMSampleBufferRef sampleBuffer = nullptr;
    result = CMSampleBufferCreateForImageBuffer(kCFAllocatorDefault, pixelBuffer, true, NULL, NULL, videoInfo, &timing, &sampleBuffer);
    NSParameterAssert(result == 0 && sampleBuffer != nullptr);
    CFRelease(videoInfo);

    CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, YES);
    auto dict = (CFMutableDictionaryRef) CFArrayGetValueAtIndex(attachments, 0);
    CFDictionarySetValue(dict, kCMSampleAttachmentKey_DisplayImmediately, kCFBooleanTrue);

    if (self.displayLayer.status == AVQueuedSampleBufferRenderingStatusFailed) {
        [self.displayLayer flush];
    }
    [self.displayLayer enqueueSampleBuffer:sampleBuffer];
    CFRelease(sampleBuffer);
}

- (void)removeObserver
{
    if (strcmp(dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL), dispatch_queue_get_label(dispatch_get_main_queue())) == 0) {
        removed = true;
        if (self.displayLayer) {
            [self.displayLayer removeFromSuperlayer];
            [parentLayer removeObserver:self forKeyPath:@"bounds" context:nil];
        }
    } else {
        dispatch_sync(dispatch_get_main_queue(), ^{
          removed = true;
          if (self.displayLayer) {
              [self.displayLayer removeFromSuperlayer];
              [parentLayer removeObserver:self forKeyPath:@"bounds" context:nil];
          }
        });
    }
}

- (void)dealloc
{
    if (_bGColour) {
        CGColorRelease(_bGColour);
    }
    if (renderingBuffer) {
        CVPixelBufferRelease(renderingBuffer);
    }
}

@end
