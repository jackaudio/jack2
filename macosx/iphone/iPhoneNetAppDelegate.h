//
//  iPhoneNetAppDelegate.h
//  iPhoneNet
//
//  Created by Stéphane  LETZ on 16/02/09.
//  Copyright Grame 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface iPhoneNetAppDelegate : NSObject <UIApplicationDelegate> {
   // UIWindow *window;
    
    IBOutlet UIWindow				*window;
	IBOutlet UINavigationController	*navigationController;
}

//@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) UIWindow *window;
@property (nonatomic, retain) UINavigationController *navigationController;

@end

