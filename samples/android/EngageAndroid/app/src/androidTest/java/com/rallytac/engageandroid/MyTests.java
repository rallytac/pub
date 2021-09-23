package com.rallytac.engageandroid;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isCompletelyDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static org.junit.Assert.assertEquals;

import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import androidx.test.espresso.Espresso;
import androidx.test.espresso.UiController;
import androidx.test.espresso.ViewAction;
import androidx.test.espresso.ViewAssertion;
import androidx.test.espresso.ViewInteraction;
import androidx.test.espresso.action.MotionEvents;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.hamcrest.Matcher;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.Random;

@RunWith(AndroidJUnit4.class)
public class MyTests {
    public static ViewAction touchDownAndUp(final int delayMs) {
        return new ViewAction() {
            @Override
            public Matcher<View> getConstraints() {
                return isDisplayed();
            }

            @Override
            public String getDescription() {
                return "Send touch events.";
            }

            @Override
            public void perform(UiController uiController, final View view) {
                int[] location = new int[2];
                view.getLocationOnScreen(location);

                float[] coordinates = new float[] {location[0] + (view.getWidth() / 2), location[1] + (view.getHeight() / 2) };
                float[] precision = new float[] { 1f, 1f };

                MotionEvent down = MotionEvents.sendDown(uiController, coordinates, precision).down;
                uiController.loopMainThreadForAtLeast(delayMs);
                MotionEvents.sendUp(uiController, down, coordinates);
            }
        };
    }

    private ViewInteraction waitForView(int id) {
        while( true ) {
            try {
                ViewInteraction rc = onView(withId(id)).check(matches(isCompletelyDisplayed()));
                return rc;
            } catch (Exception e) {
            }

            try {
                Thread.sleep(100);
            }
            catch (Exception e) {
            }
        }
    }

    @Test
    public void shortCountPttTest() {
        hammerPtt(10);
    }

    @Test
    public void mediumCountPttTest() {
        hammerPtt(100);
    }

    @Test
    public void longCountPttTest() {
        hammerPtt(1000);
    }

    @Test
    public void veryLongCountPttTest() {
        hammerPtt(10000);
    }

    @Test
    public void overnightPttTest() {
        hammerPtt(100000);
    }

    private void hammerPtt(int loopCount) {
        Random rnd = new Random();
        int pauseBetweenPtt;
        int pttTime;

        long loops = 0;

        Log.d("myTests:hammerPtt", "starting PTT hammer for " + loopCount + " iterations");

        while( loops < loopCount ) {
            try {
                onView(withId(R.id.ivPtt)).check(matches(isCompletelyDisplayed()));

                pauseBetweenPtt = 100 + rnd.nextInt(20);
                pttTime = 1000 + rnd.nextInt(3000);

                Log.d("myTests:hammerPtt", "ptt for " + pttTime + " milliseconds, followed by pause of " + pauseBetweenPtt + " milliseconds");

                onView(withId(R.id.ivPtt)).perform(touchDownAndUp(pttTime));
                loops++;
                if(loops % 10 == 0) {
                    Log.d("myTests:hammerPtt", loops + " loops out of " + loopCount);
                }
            }
            catch (Exception e) {
                pauseBetweenPtt = 100;
                Log.e("myTests:hammerPtt", e.getMessage());
            }

            try {
                Thread.sleep(pauseBetweenPtt);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    @Test
    public void toggleSingleMulti() {
        Random rnd = new Random();
        int loopCount = 10000;
        int pauseBetweenToogle;

        long loops = 0;

        Log.d("myTests:toggleSingleMulti", "starting PTT hammer for " + loopCount + " iterations");

        while( loops < loopCount ) {
            try {
                //onView(withId(R.id.tvGroupName)).check(matches(isCompletelyDisplayed()));

                pauseBetweenToogle = 1000 + rnd.nextInt(2000);

                //Log.d("myTests:toggleSingleMulti", "ptt for " + pttTime + " milliseconds, followed by pause of " + pauseBetweenPtt + " milliseconds");

                //onView(withId(R.id.tvGroupName)).perform(touchDownAndUp(100));
                //onView(withId(R.id.tvGroupName)).perform(touchDownAndUp(100));

                Espresso.pressBack();

                loops++;
                if(loops % 10 == 0) {
                    Log.d("myTests:toggleSingleMulti", loops + " loops out of " + loopCount);
                }
            }
            catch (Exception e) {
                pauseBetweenToogle = 100;
                Log.e("myTests:toggleSingleMulti", e.getMessage());
            }

            try {
                Thread.sleep(pauseBetweenToogle);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }    
}
