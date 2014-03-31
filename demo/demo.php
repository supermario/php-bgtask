<?php

/*
 * A basic demo of the bgTask class capabilities.
 */

// Ensure errors are displayed for the demo
ini_set('error_reporting', E_ALL);
ini_set('display_errors', true);


// include the bgTask class
require('../bgtask.class.inc');

// we'll be running demo.sh asyncronously in the background
$demoTaskCommand = './demo.sh';

// let's load a new bgTask object with a specified alias
$demoTask = new bgTask('demoTask');

// you can specify an absolute path to a temp folder for bgtask to use if you wish
// default behaviour is to attempt creation of a 'bgtasks' folder in the same directory as the class file
// $demoTask = new bgTask('demoTask', '/tmp/bgtasks');



// Now that we've loaded/polled our task, we can run different behaviour based on it's status
// See the class definition for a full list of status constants

if ($demoTask->status() == bgTask::STATUS_EMPTY) {

    // Task is empty and ready to run a new command!
    $demoTask->exec($demoTaskCommand);
    print "We've started a new bgTask! Refresh to query it again and see any output it might have.";

} else if ($demoTask->status() == bgTask::STATUS_COMPLETE) {

    // Task has completed
    print "Our task has completed. It's final output was:";
    print "<pre>".$demoTask->output()."</pre>";

    // Once a task has been polled with a result of STATUS_COMPLETE, all temp files for that bgTask are destroyed
    // This means it's up to you to save the output/exitcode somewhere (i.e. into a database) if you wish to retain it further
    // In this demo we simply display it to the user, and no history gets kept

} else if ($demoTask->status() == bgTask::STATUS_ERROR) {

    print "There was an error with our task. It's output was:";
    print "<pre>".$demoTask->output()."</pre>";
    print "and it terminated with an exit code of ".$demoTask->exitcode();

} else {

    // Task is already running, print out it's output thus far
    print "Our task is already running. Here is it's output so far:";
    print "<pre>".$demoTask->output()."</pre>";
    print "Continue refreshing to check for futher output and completion.";

}
