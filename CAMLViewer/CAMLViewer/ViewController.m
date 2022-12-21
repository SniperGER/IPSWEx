//
//  ViewController.m
//  CAMLViewer
//
//  Created by Team FESTIVAL on 07.06.22.
//

#import "ViewController.h"
#import "CALayer+Private.h"
#import "CAPackage.h"
#import "CAStateController.h"
#import "FLEXManager.h"

extern NSString* const kCAPackageTypeCAMLBundle;

@interface ViewController () {
	UIView* composingView;
	NSDictionary* screenClasses;
	NSDictionary* wallpaper;
	NSDictionary* screenInfo;

	NSMutableArray<CAPackage*>* packages;
}

@end

@implementation ViewController

- (BOOL)prefersStatusBarHidden {
	return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
	return YES;
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];

	screenClasses = [NSDictionary dictionaryWithContentsOfURL:[NSBundle.mainBundle URLForResource:@"ScreenSizes" withExtension:@"plist"]];
	composingView = [[UIView alloc] initWithFrame:CGRectZero];

	[self composeWallpaper];
}

- (void)composeWallpaper {
	[composingView.layer.sublayers makeObjectsPerformSelector:@selector(removeFromSuperlayer)];

	if (![NSFileManager.defaultManager fileExistsAtPath:[NSString stringWithFormat:@"%s/Wallpaper.plist", WALLPAPER_ROOT]]) {
		[self showAlertWithTitle:@"No Wallpaper" message:@"No wallpaper found to render."];

		return;
	}

	wallpaper = [NSDictionary dictionaryWithContentsOfFile:[NSString stringWithFormat:@"%s/Wallpaper.plist", WALLPAPER_ROOT]];

	if (!wallpaper) {
		[self showAlertWithTitle:@"Failed to load wallpaper" message:@"Failed to load wallpaper."];

		return;
	}

	if ([screenClasses.allKeys indexOfObject:wallpaper[@"logicalScreenClass"]] == NSNotFound) {
		[self showAlertWithTitle:@"Unknown Screen Size" message:[NSString stringWithFormat:@"Size class „%@“ is unknown.", wallpaper[@"logicalScreenClass"]]];

		return;
	}

	screenInfo = screenClasses[wallpaper[@"logicalScreenClass"]];


	NSArray* keys = @[
		@"backgroundAnimationFileName",
		@"foregroundAnimationFileName",
		@"floatingAnimationFileNameKey",
	];

	packages = [NSMutableArray array];

	[keys enumerateObjectsUsingBlock:^(NSString* key, NSUInteger idx, BOOL * stop) {
		NSString* value = [wallpaper valueForKeyPath:[NSString stringWithFormat:@"assets.lockAndHome.default.%@", key]];

		if (value) {
			CAPackage* package = [CAPackage packageWithContentsOfURL:[NSURL fileURLWithPath:[NSString stringWithFormat:@"%s/%@", WALLPAPER_ROOT, value]] type:kCAPackageTypeCAMLBundle options:nil error:NULL];

			if (package) {
				[packages addObject:package];
			}
		}
	}];

	[composingView setFrame:(CGRect){ CGPointZero, { [screenInfo[@"width"] floatValue], [screenInfo[@"height"] floatValue] }}];
	[self.view addSubview:composingView];
	[self.view sendSubviewToBack:composingView];

	NSMutableArray<CAStateController*>* controllers = [NSMutableArray array];

	[packages enumerateObjectsUsingBlock:^(CAPackage* package, NSUInteger index, BOOL* stop) {
		[package.rootLayer setGeometryFlipped:NO];

		CAStateController* controller = [[CAStateController alloc] initWithLayer:package.rootLayer];
//		[controller setInitialStatesOfLayer:package.rootLayer transitionSpeed:1.0];
		[controller setState:[package.rootLayer stateWithName:@"Sleep"] ofLayer:package.rootLayer];
		[controllers addObject:controller];

		[composingView.layer addSublayer:package.rootLayer];
	}];
}

- (IBAction)reloadWallpaper:(id)sender {
	if (packages) {
		[packages enumerateObjectsUsingBlock:^(CAPackage* package, NSUInteger index, BOOL* stop) {
			[package.rootLayer removeFromSuperlayer];
		}];
	}

	[self composeWallpaper];
}

- (IBAction)exportWallpaper:(id)sender {
	NSDictionary* exportOptions = [NSDictionary dictionaryWithContentsOfFile:[NSString stringWithFormat:@"%s/../ExportOptions.plist", WALLPAPER_ROOT]];

	CGSize viewSize = composingView.bounds.size;
	
	UIGraphicsBeginImageContextWithOptions(viewSize, NO, [screenInfo[@"screenScale"] floatValue]);
	[composingView drawViewHierarchyInRect:CGRectMake(0, 0, viewSize.width, viewSize.height) afterScreenUpdates:YES];
	UIImage* composedImage = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();

	NSString* path = [NSString stringWithFormat:@"%s/%@/%@/%@/Stills/%@-%@.heic", WALLPAPER_OUTPUT, screenInfo[@"deviceClass"], screenInfo[@"outputDir"], exportOptions[@"IOS_VERSION"], exportOptions[@"WALLPAPER_NAME"], wallpaper[@"logicalScreenClass"]];

	if (![NSFileManager.defaultManager fileExistsAtPath:[path stringByDeletingLastPathComponent]]) {
		[NSFileManager.defaultManager createDirectoryAtPath:[path stringByDeletingLastPathComponent]
								  withIntermediateDirectories:YES
												   attributes:nil
														error:NULL];
	}

	CFMutableDataRef mutableData = CFDataCreateMutable(nil, 0);
	CGImageDestinationRef destination = CGImageDestinationCreateWithData(mutableData, CFSTR("public.heic"), 1, nil);
	CGImageDestinationAddImage(destination, composedImage.CGImage, (CFDictionaryRef)@{
		(__bridge NSString*)kCGImageDestinationLossyCompressionQuality: @1.0f,
		(__bridge NSString*)kCGImagePropertyOrientation: @(kCGImagePropertyOrientationUp)
	});
	CGImageDestinationFinalize(destination);

	[(__bridge NSData*)mutableData writeToFile:path atomically:YES];

	NSLog(@"Wallpaper saved to %@", path);
	[self showAlertWithTitle:@"Export finished" message:[NSString stringWithFormat:@"Wallpaper has been saved to %@", path]];
}

- (void)showAlertWithTitle:(NSString*)title message:(NSString*)message {
	UIAlertController* alertController = [UIAlertController alertControllerWithTitle:title
																			 message:message
																	  preferredStyle:UIAlertControllerStyleAlert];
	[alertController addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];

	[self presentViewController:alertController animated:YES completion:nil];
}

@end
