//
//  RGBPickAppDelegate.m
//  rgbmacpick
//
//  Created by Boris on 11-10-18.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "librgblamp.h"
#import "RGBPickAppDelegate.h"

@implementation RGBPickAppDelegate

//@synthesize window = _window;

- (void)dealloc
{
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    rgb_open("/dev/cu.usbserial");

    NSColorPanel *cp = [ NSColorPanel sharedColorPanel ];

    [ cp setTarget: self ];
    [ cp setDelegate: self ];
    [ cp setContinuous: true ];
    [ cp setShowsAlpha: false ];

    double curcolor[3];
    if (rgb_getout_srgb(curcolor)) {
        NSColor *curcolns = [ NSColor colorWithSRGBRed:curcolor[0] green:curcolor[1] blue:curcolor[2] alpha:1.0 ];
        [ cp setColor: curcolns ];
    }

    [ NSColorPanel setPickerMode: NSWheelModeColorPanel ];
    [ cp makeKeyAndOrderFront: nil ];
}

// Color updates from panel
- (void)changeColor:(id)sender
{
    NSColor *color = [ sender color ];
    NSColor *srgbcolor = [ color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
    CGFloat r, g, b, a;

    [ srgbcolor getRed:&r green:&g blue:&b alpha:&a ];
    rgb_pwm_srgb(r, g, b);
    //NSLog(@"changeColor");
}

// Notification that color panel will close
- (void)windowWillClose:(NSNotification*)note
{
    rgb_fadeprep();
    rgb_close();
    [NSApp terminate:self];
}

@end
