package io.veyon.Veyon.Server;

import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.GestureDescription;
import android.view.accessibility.AccessibilityEvent;
import android.view.ViewConfiguration;
import android.graphics.Path;

public class VeyonAccessibilityService extends AccessibilityService {

	private static VeyonAccessibilityService instance;

	@Override
	public void onAccessibilityEvent( AccessibilityEvent event ) { }

	@Override
	public void onInterrupt() { }

	@Override
	public void onCreate()
	{
		super.onCreate();
		instance = this;
	}

	public static void tap( int x, int y )
	{
		instance.dispatchGesture( createClick( x, y, ViewConfiguration.getTapTimeout() + 50 ), null, null );
	}

	public static void longPress( int x, int y )
	{
		instance.dispatchGesture( createClick( x, y, ViewConfiguration.getLongPressTimeout() + 200 ), null, null );
	}

	public static void swipeDown( int x, int y )
	{
		instance.dispatchGesture( createSwipe( x, y, x, y+100 ), null, null );
	}

	public static void swipeUp( int x, int y )
	{
		instance.dispatchGesture( createSwipe( x, y, x, y-100 ), null, null );
	}

	private static GestureDescription createClick( int x, int y, int duration )
	{
 		Path clickPath = new Path();
		clickPath.moveTo( x, y );
		GestureDescription.StrokeDescription clickStroke = new GestureDescription.StrokeDescription( clickPath, 0, duration );
		GestureDescription.Builder clickBuilder = new GestureDescription.Builder();
		clickBuilder.addStroke( clickStroke );
		return clickBuilder.build();
	}

	private static GestureDescription createSwipe( int x1, int y1, int x2, int y2 )
	{
 		Path swipePath = new Path();
		swipePath.moveTo( x1, y1 );
		swipePath.moveTo( x2, y2 );
		GestureDescription.StrokeDescription swipeStroke = new GestureDescription.StrokeDescription( swipePath, 0, 100 );
		GestureDescription.Builder swipeBuilder = new GestureDescription.Builder();
		swipeBuilder.addStroke( swipeStroke );
		return swipeBuilder.build();
	}
}
