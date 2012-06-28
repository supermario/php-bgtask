<?php

/*
 * bgTask : a simple PHP interface for executing unattended (non-blocking) system calls,
 * monitoring their output and exit status.
 *
 * @security
 *      In it's plain form this class allows a developer to run any desired command via PHP's exec()
 *      Allowing any user input to this class is allowing direct execution of user controlled input on your server
 *
 * @author Mario Rogic, hello@mario.net.au
 *
 */

class bgTask {
    const STATUS_EMPTY = 'empty';
    const STATUS_STARTING = 'starting';
    const STATUS_RUNNING = 'running';
    const STATUS_COMPLETE = 'complete';
    const STATUS_ERROR = 'error';

    // status is determined once at creation of the object
    // to re-query the task status a new object needs to be created
    private $status = bgTask::STATUS_EMPTY;
    // the pid of this task, if it has commenced running
    private $pid = null;
    // the output of this task as at creation of the object
    private $output = "";
    // the exit code of this task as at creation of the object
    private $exitcode = null;

    // the temporary files where task data will be stored
    private $outfile = "";
    private $pidfile = "";
    private $exitfile = "";

    // the user named alias for this task, used to retreive task data
    private $alias = "";

    // the directory that will be used to store temporary files
    // web user must have permissions on this folder, will attempt to create if non-existent
    private $tempdir = null;

    public function __construct($alias, $tempdir = null) {

        // if no tempdir has been specified, attempt to use 'bgtasks' folder in same location as class file
        $this->tempdir = (is_null($tempdir)) ? dirname(__FILE__) . "/bgtasks" : $tempdir ;

        $this->alias = $alias;
        // create tempdir if it doesn't exists, check it's writeable
        if (!file_exists($this->tempdir)) { mkdir($this->tempdir); }
        if (!is_writable($this->tempdir)) { trigger_error("bgtask temp directory ({$this->tempdir}) not writeable", E_USER_ERROR); }
        // hash the alias for both privacy and security
        // (otherwise user input is directly run in exec)
        $hash = md5($this->alias);
        // set our out/pid/exit code filenames
        $this->outfile = $this->tempdir . "/" . $hash . ".out.txt";
        $this->pidfile = $this->tempdir . "/" . $hash . ".pid.txt";
        $this->exitfile = $this->tempdir . "/" . $hash . ".exit.txt";
        // attempt to load the pid and set the task status
        $this->setPid();
        $this->setStatus();
    }
    // returns the status of the task as at creation of bgTask object
    public function status() {
        return $this->status;
    }
    // returns the output of the task as at creation of bgTask object
    public function output() {
        return $this->output;
    }
    // returns the exit code of the task if task was complete at creation of bgTask object
    public function exitcode() {
        return $this->exitcode;
    }
    public function exec($cmd) {
        if (empty($this->pid)) {
            // Thanks to William Pursell for this lovely solution
            // Non-blocking shell script to:
            //      - execute a command
            //      - write it's pid to a file immediately
            //      - write it's output to a file as it runs
            //      - write it's exit code to a file upon completion
            // http://stackoverflow.com/questions/9261397/bash-get-process-id-and-exit-code
            $cmd_prepared = sprintf(
                "echo \"Executing: %s\" > %s    # Write command to be executed to outfile
                { sh -c \"%s >> %s 2>&1 &\"'    # Run given command and redirect all output to outfile (backgrounded)
                    echo $! > %s                # Write pid to pidfile
                    echo                        # Alert parent that the pid has been written
                    wait $!                     # Wait for process with pid to complete
                    echo $? > %s                # Write exit status to file
                    ' & } | read                # Background previous command set, block on read until pidfile exists"
            , $cmd, $this->outfile, $cmd, $this->outfile, $this->pidfile, $this->exitfile);
            exec($cmd_prepared, $cmd_out);
            $this->status = bgTask::STATUS_STARTING;
        } else {
            // user cannot start a new task with the same alias until the old task has reported completion (STATUS_COMPLETE)
            trigger_error("bgTask '{$this->alias}' is still active", E_USER_ERROR);
        }
    }
    // internally loads and stores the pid value for this task if possible
    private function setPid() {
        $this->pid = null;
        if (file_exists($this->pidfile)) {
            $this->pid = file_get_contents($this->pidfile);
        }
    }
    // internally determines and stores the status for this task at the current point in time
    private function setStatus() {
        if (empty($this->pid)) {
            $this->status = bgTask::STATUS_EMPTY;
        } else {
            $this->loadOutput();
            $this->loadExitcode();

            $result = shell_exec(sprintf("ps %d", $this->pid));
            if (count(preg_split("/\n/", $result)) > 2){
                $this->status = bgTask::STATUS_RUNNING;
            } else {
                if ($this->exitcode == 0) {
                    $this->status = bgTask::STATUS_COMPLETE;
                } else {
                    $this->status = bgTask::STATUS_ERROR;
                }

                $this->clearFiles();
            }
        }
    }
    // internally loads and stores the task's output up till the current point in time
    private function loadOutput() {
        if (file_exists($this->outfile)) {
            $this->output = file_get_contents($this->outfile);
        }
    }
    // internally loads and stores the task's exit code if it's completed
    private function loadExitcode() {
        if (file_exists($this->exitfile)) {
            $this->exitcode = file_get_contents($this->exitfile);
        }
    }
    // clears all temporary files
    private function clearFiles() {
        unlink($this->pidfile);
        unlink($this->outfile);
        unlink($this->exitfile);
    }
}
