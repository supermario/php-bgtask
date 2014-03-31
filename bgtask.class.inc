<?php

/*
 * bgTask : a simple PHP interface for executing unattended (non-blocking) system calls,
 * monitoring their output and exit status.
 *
 * @usage
 *      See demo.php in the demo folder for usage example and basic documentation
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

    // Status is determined once at creation of the object
    // To re-query the task status a new object needs to be created
    private $status = null;
    // The pid of this task, if it has commenced running
    private $pid = null;
    // The output of this task as at creation of the object
    private $output = "";
    // The exit code of this task as at creation of the object
    private $exitcode = null;

    // The temporary files where task data will be stored
    private $outfile = "";
    private $pidfile = "";
    private $exitfile = "";

    // The user named alias for this task, used to retreive task data
    private $alias = "";

    // The directory that will be used to store temporary files
    // Web user must have permissions on this folder, will attempt to create if non-existent
    private $tempdir = null;

    public function __construct($alias, $tempdir = null) {

        // If no tempdir has been specified, attempt to use 'bgtasks' folder in same location as class file
        $this->tempdir = (is_null($tempdir)) ? dirname(__FILE__) . "/bgtasks" : $tempdir ;

        $this->alias = $alias;

        // Create tempdir if it doesn't exist, check it's writeable
        if (!file_exists($this->tempdir)) { mkdir($this->tempdir); }
        if (!is_writable($this->tempdir)) { trigger_error("bgtask temp directory ({$this->tempdir}) not writeable", E_USER_ERROR); }
        // Hash the alias for both privacy and security
        // (otherwise user input is directly run in exec)
        $hash = md5($this->alias);
        // Set our out/pid/exit code filenames
        $this->outfile = $this->tempdir . "/" . $hash . ".out.txt";
        $this->pidfile = $this->tempdir . "/" . $hash . ".pid.txt";
        $this->exitfile = $this->tempdir . "/" . $hash . ".exit.txt";
        // Attempt to load the pid and set the task status
        $this->setPid();
        $this->setStatus();
    }
    // returns the status of the task
    // this value is static as of creation of bgTask object, or latest reload
    // to recheck the status and output, see reload()
    public function status() {
        return $this->status;
    }
    // reloads the bgtask status code and output, returns the status
    // do not rely on this method to get the status repeatedly, only use it when
    // you want to explicitly reload the output and pid status during script
    // execution, else you may suffer synchronicity issues
    public function reload() {
        $this->setPid();
        $this->setStatus();
        return $this->status();
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
			if ($this->function_exists('exec'))
				@exec($cmd_prepared);
			elseif ($this->function_exists('system'))
				@system($cmd_prepared);
			elseif ($this->function_exists('passthru'))
				@passthru($cmd_prepared);
            else
                trigger_error("bgTask found no available system call commands, please see README and check your environment config", E_USER_ERROR);
            $this->status = bgTask::STATUS_STARTING;
        } else {
            // user cannot start a new task with the same alias until the old task has reported completion (STATUS_COMPLETE)
            trigger_error("bgTask '{$this->alias}' is still active", E_USER_ERROR);
        }
    }

	// Checks if a function exists in the current environment. Some shared hosting
	// environments disable certain functions
	private function function_exists($func) {
		if(!function_exists($func))
			return false;

		$disabled = explode(', ', @ini_get('disable_functions'));
		if (in_array($func, $disabled))
			return false;

		if (extension_loaded('suhosin')) {
			$suhosin = @ini_get("suhosin.executor.func.blacklist");
			if (empty($suhosin) == false) {
				$suhosin = explode(',', $suhosin);
				$blacklist = array_map('trim', $suhosin);
				$blacklist = array_map('strtolower', $blacklist);
				if(in_array($func, $blacklist))
					return false;
			}
		}
        // Function appears to be available
		return true;
	}

    // Loads and stores the pid value for this task if possible
    private function setPid() {
        $this->pid = null;
        if (file_exists($this->pidfile)) {
            $this->pid = file_get_contents($this->pidfile);
        }
    }
    // Determines and stores the status for this task at the current point in time
    private function setStatus() {
        if (empty($this->pid)) {
            $this->status = bgTask::STATUS_EMPTY;
        } else if ($this->status == bgTask::STATUS_COMPLETE || $this->status == bgTask::STATUS_EMPTY) {
            // When reloading a completed or empty task, new status is empty
            $this->status = bgTask::STATUS_EMPTY;
        } else {
            $this->loadOutput();
            $this->loadExitcode();

            // Get the process status of our task by pid
            $result = shell_exec(sprintf("ps %d", $this->pid));
            if (count(preg_split("/\n/", $result)) > 2){
                $this->status = bgTask::STATUS_RUNNING;
            } else {
                if ($this->exitcode == 0) {
                    $this->status = bgTask::STATUS_COMPLETE;
                } else {
                    $this->status = bgTask::STATUS_ERROR;
                }
                // Now that the task is complete, clear out temp files
                $this->clearFiles();
            }
        }
    }
    // Loads and stores the task's output up till the current point in time
    private function loadOutput() {
        if (file_exists($this->outfile)) {
            $this->output = file_get_contents($this->outfile);
        }
    }
    // Loads and stores the task's exit code if it's completed
    private function loadExitcode() {
        if (file_exists($this->exitfile)) {
            $this->exitcode = file_get_contents($this->exitfile);
        }
    }
    // Clears all temporary files
    private function clearFiles() {
        unlink($this->pidfile);
        unlink($this->outfile);
        unlink($this->exitfile);
    }
}
