
/* Copyright 2008 Cluster Resources */

#include <pbs_config.h>   /* the master config generated by configure */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "dis.h"
#include "libpbs.h"
#include "pbs_error.h"
#include "job.h"
#include "batch_request.h"
#include "attribute.h"
#include "mom_mach.h"
#include "mom_func.h"
#include "log.h"
#include "mcom.h"
#include "net_connect.h"
#include "resource.h"
#include "csv.h"
#include "svrfunc.h"

extern int   exiting_tasks;
extern int LOGLEVEL;
extern     int             lockfds;

char	       path_checkpoint[1024];

/* BLCR variables */
char                    checkpoint_script_name[1024];
char                    restart_script_name[1024];
char                    checkpoint_run_exe_name[1024];

extern char *mk_dirs A_((char *));


/**
 * mom_checkpoint_job_is_checkpointable
 *
 * @param pjob Pointer to job structure.
 * @see TMomFinalizeChild
 */


int
mom_checkpoint_job_is_checkpointable(job *pjob)
  {
  return(mom_does_checkpoint());
  }


/**
 * mom_checkpoint_execute_job
 *
 * @param pjob Pointer to job structure.
 * @see TMomFinalizeChild
 */
void
mom_checkpoint_execute_job(job *pjob,char *shell,char *arg[],struct var_table *vtable)
  {

  /* Launch job executable with cr_run command so that cr_checkpoint command will work. */

  arg[3] = arg[2];                     /* shuffle up the existing args */
  arg[2] = arg[1];
  arg[1] = malloc(strlen(shell) + 1);  /* replace first arg with shell name */

  strcpy(arg[1],shell);

  execve(checkpoint_run_exe_name,arg,vtable->v_envp);
  }


/**
 * mom_checkpoint_init
 *
 * This routine is called from the mom startup code.
 * @see setup_program_environment
 */
int
mom_checkpoint_init()
  {
  int c = 0;

  if (strlen(path_checkpoint) == 0)	/* if not -C option */
    strcpy(path_checkpoint, mk_dirs("checkpoint/"));


#if !defined(DEBUG) && !defined(NO_SECURITY_CHECK)

  c = chk_file_sec(path_checkpoint,1,0,S_IWGRP|S_IWOTH,1,NULL);

#endif  /* not DEBUG and not NO_SECURITY_CHECK */
  return(c);
  }



/*========================================================================*/
/* Routines called from the config file parsing to set various variables. */
/*========================================================================*/


void
mom_checkpoint_set_directory_path(char *str)
  {
  char *cp;

  strcpy(path_checkpoint,str);
  cp = &path_checkpoint[strlen(path_checkpoint)];
  if (*cp != '/')
    {
      *cp++ = '/';
      *cp++ = 0;
    }
  }


unsigned long
mom_checkpoint_set_checkpoint_script(char *value)  /* I */

  {
  struct stat sbuf;

  log_record(
    PBSEVENT_SYSTEM,
    PBS_EVENTCLASS_SERVER,
    "checkpoint_script",
    value);

  if ((stat(value,&sbuf) == -1) || !(sbuf.st_mode & S_IXUSR))
    {
    /* file does not exist or is not executable */

    return(0);  /* error */
    }

  strncpy(checkpoint_script_name, value, sizeof(checkpoint_script_name));

  return(1);
  }  /* END set_checkpoint_script() */





unsigned long
mom_checkpoint_set_restart_script(char *value)  /* I */

  {
  struct stat sbuf;

  log_record(
    PBSEVENT_SYSTEM,
    PBS_EVENTCLASS_SERVER,
    "restart_script",
    value);

  if ((stat(value, &sbuf) == -1) || !(sbuf.st_mode & S_IXUSR))
    {
    /* file does not exist or is not executable */

    return(0);  /* error */
    }

  strncpy(restart_script_name, value, sizeof(restart_script_name));

  return(1);
  }  /* END set_restart_script() */





unsigned long
mom_checkpoint_set_checkpoint_run_exe_name(char *value)  /* I */

  {
  struct stat sbuf;

  log_record(
    PBSEVENT_SYSTEM,
    PBS_EVENTCLASS_SERVER,
    "checkpoint_run_exe",
    value);

  if ((stat(value, &sbuf) == -1) || !(sbuf.st_mode & S_IXUSR))
    {
    /* file does not exist or is not executable */

    return(0);  /* error */
    }

  strncpy(checkpoint_run_exe_name, value, sizeof(checkpoint_run_exe_name));

  return(1);
  }  /* END set_checkpoint_run_exe() */






/**
 * mom_checkpoint_delete_files
 *
 * This routine is called from the job_purge routine
 * which cleans up all files related to a job.
 *
 * @param pjob Pointer to the job structure
 * @see job_purge
 */
void
mom_checkpoint_delete_files(job *pjob)
  {
  /* Leave the checkpoint files around per request by Cray */
#if 0
  extern char *path_checkpoint;

  strcpy(namebuf,path_checkpoint);	/* delete any checkpoint file */
  strcat(namebuf,pjob->ji_qs.ji_fileprefix);
  strcat(namebuf,JOB_CKPT_SUFFIX);
  remtree(namebuf);
#endif
  }


/**
 * mom_checkpoint_recover
 *
 * This routine is called from init_abort_jobs which in turn is called
 * on mom startup.  The purpose is to recover jobs listed in the mom_priv/jobs
 * directory.
 *
 * This routine does not actually start the job.  This happens in start_exec.c.
 * It's purpose is to remove a partially completed checkpoint directory,
 * signified by the name suffix of ".old".
 *
 * @param pjob Pointer to job data structure
 * @see init_abort_jobs
 */
void
mom_checkpoint_recover(job *pjob)
  {
  char           path[MAXPATHLEN + 1];
  char           oldp[MAXPATHLEN + 1];
    /*
    ** Check to see if a checkpoint.old dir exists.
    ** If so, remove the regular checkpoint dir
    ** and rename the old to the regular name.
    */

    strcpy(path,path_checkpoint);
    strcat(path,pjob->ji_qs.ji_fileprefix);
    strcat(path,JOB_CHECKPOINT_SUFFIX);
    strcpy(oldp,path);
    strcat(oldp,".old");

#if 0
    /*
     * This is if 0'ed.
     * If the directory exists it is fine.
     * Checkpoint names have a time suffix and can occupy
     * the same directory space.
     */
    if (stat(oldp,&statbuf) == 0) 
      {
      remtree(path);

      if (rename(oldp,path) == -1)
        remtree(oldp);
      }
#endif
  }



/**
 * mom_checkpoint_check_periodic_timer
 *
 * This routine is called from the main loop routine examine_all_running_jobs.
 * Each job that is checkpointable will have timer variables set up.
 * This routine checks the timer variables and if set and it is time
 * to do a checkpoint, fires the code that starts a checkpoint.
 *
 * @param pjob Pointer to the job structure
 * @see examine_all_running_jobs
 * @see main_loop
 */
void
mom_checkpoint_check_periodic_timer(job *pjob)
  {
  resource	*prscput;
  extern int start_checkpoint();
  int rc;
static resource_def *rdcput;

  /* see if need to checkpoint any job */

  if (pjob->ji_checkpoint_time != 0)  /* ji_checkpoint_time gets set in start_exec */
    {
    if (rdcput == NULL)
      {
      rdcput = find_resc_def(svr_resc_def,"cput",svr_resc_size);
      }
    if (rdcput != NULL)
      {
      prscput = find_resc_entry(
        &pjob->ji_wattr[(int)JOB_ATR_resc_used],
        rdcput);  /* resource definition cput set in startup */

      if (prscput &&
         (pjob->ji_checkpoint_next > prscput->rs_value.at_val.at_long))
        {
        pjob->ji_checkpoint_next = 
          prscput->rs_value.at_val.at_long +
          pjob->ji_checkpoint_time;
  
        if ((rc = start_checkpoint(pjob,0,0)) != PBSE_NONE)
          {
          sprintf(log_buffer,"Checkpoint failed, error %d", rc);

          message_job(pjob,StdErr,log_buffer);

          log_record(
            PBSEVENT_JOB, 
            PBS_EVENTCLASS_JOB,
            pjob->ji_qs.ji_jobid, 
            log_buffer);
          }
        }
      }
    }
  }


/*
 * Checkpoint the job.
 *
 *	If abort is TRUE, kill it too.  Return a PBS error code.
 */

int mom_checkpoint_job(

  job *pjob,  /* I */
  int  abort, /* I */
  int  admin) /* I */

  {
/*  int		hasold = 0; */
  int		sesid = -1;
  int		ckerr;
/*  struct stat	statbuf; */
  char		path[MAXPATHLEN + 1];
/*  char		oldp[MAXPATHLEN + 1]; */
  char		file[MAXPATHLEN + 1]; 
  char         *name;
  int		filelen;
  task         *ptask;
  extern char   task_fmt[];

  assert(pjob != NULL);

  strcpy(path,path_checkpoint);
  strcat(path,pjob->ji_qs.ji_fileprefix);
  strcat(path,JOB_CHECKPOINT_SUFFIX);

#if 0
  /* Don't mess with existing checkpoints. */
  if (stat(path,&statbuf) == 0) 
    {
    strcpy(oldp,path);   /* file already exists, rename it */

    strcat(oldp,".old");

    if (rename(path,oldp) < 0)
      {
      return(errno);
      }

    hasold = 1;
    }
#endif

  mkdir(path,0755);

  filelen = strlen(path);

  strcpy(file,path);

  name = &file[filelen];

#ifdef _CRAY

  /*
   * if job is suspended and if <abort> is set, resume job first,
   * this is so job will be "Q"ueued and then back into "R"unning
   * when restarted.
   */

  if ((pjob->ji_qs.ji_svrflags & JOB_SVFLG_Suspend) && abort) 
    {
    for (ptask = (task *)GET_NEXT(pjob->ji_tasks); 
         ptask != NULL; 
         ptask = (task *)GET_NEXT(ptask->ti_jobtask)) 
      {
      sesid = ptask->ti_qs.ti_sid;

      if (ptask->ti_qs.ti_status != TI_STATE_RUNNING)
        continue;

      /* What to do if some resume work and others don't? */

      if ((ckerr = resume(C_JOB,sesid)) == 0) 
        {
        post_resume(pjob, ckerr);
        } 
      else 
        {
        sprintf(log_buffer,"checkpoint failed: errno=%d sid=%d",
          errno, 
          sesid);

        LOG_EVENT(
          PBSEVENT_JOB, 
          PBS_EVENTCLASS_JOB,
          pjob->ji_qs.ji_jobid, 
          log_buffer);

        return(errno);
        }
      }
    }
#endif	/* _CRAY */

  for (ptask = (task *)GET_NEXT(pjob->ji_tasks);
       ptask != NULL;
       ptask = (task *)GET_NEXT(ptask->ti_jobtask)) 
    {
    sesid = ptask->ti_qs.ti_sid;

    if (ptask->ti_qs.ti_status != TI_STATE_RUNNING)
      continue;

    sprintf(name,task_fmt, 
      ptask->ti_qs.ti_task);

    if (mach_checkpoint(ptask,file,abort,admin) == -1)
      goto fail;
    }

  /* Checkpoint successful */

  pjob->ji_qs.ji_svrflags |= JOB_SVFLG_CHECKPOINT_FILE;

  job_save(pjob,SAVEJOB_FULL);  /* to save resources_used so far */

  sprintf(log_buffer,"checkpointed to %s", 
    path);

  log_record(
    PBSEVENT_JOB, 
    PBS_EVENTCLASS_JOB,
    pjob->ji_qs.ji_jobid, 
    log_buffer);

#if 0
  if (hasold) 
    remtree(oldp);
#endif

  return(PBSE_NONE);

fail:

  /* A checkpoint has failed.  Log and return error. */

  ckerr = errno;

  sprintf(log_buffer,"checkpoint failed:errno=%d sid=%d", 
    errno, 
    sesid);

  LOG_EVENT(
    PBSEVENT_JOB, 
    PBS_EVENTCLASS_JOB,
    pjob->ji_qs.ji_jobid,
    log_buffer);

  /*
  ** See if any checkpoints worked and abort is set.
  ** If so, we need to restart these tasks so the whole job is
  ** still running.  This has to wait until we reap the
  ** aborted task(s).
  */

  if (abort)
    {
    return(PBSE_CKPSHORT);
    }

  /* Clean up files */
#if 0
  remtree(path);

  if (hasold) 
    {
    if (rename(oldp,path) == -1)
      pjob->ji_qs.ji_svrflags &= ~JOB_SVFLG_CHECKPOINT_FILE;
    }
#endif

  if (ckerr == EAGAIN)
    {
    return(PBSE_CKPBSY);
    }

  return(ckerr);
  }





/* 
 * post_checkpoint - post processor for start_checkpoint()
 *
 *	Called from scan_for_terminated() when found in ji_mompost;
 *	This sets the "has checkpoint image" bit in the job.
 */

void post_checkpoint(

  job *pjob,
  int  ev)

  {
  char           path[MAXPATHLEN + 1];
  DIR		*dir;
  struct dirent	*pdir;
  tm_task_id	tid;
  task		*ptask;
  int		abort = pjob->ji_flags & MOM_CHECKPOINT_ACTIVE;

  exiting_tasks = 1;	/* make sure we call scan_for_exiting() */

  pjob->ji_flags &= ~MOM_CHECKPOINT_ACTIVE;

  if (ev == 0) 
    {
    pjob->ji_qs.ji_svrflags |= JOB_SVFLG_CHECKPOINT_FILE;

    return;
    }

  /*
  ** If we get here, an error happened.  Only try to recover
  ** if we had abort set.
  */

  if (abort == 0)
    {
    return;
    }

  /*
  ** Set a flag for scan_for_exiting() to be able to
  ** deal with a failed checkpoint rather than doing
  ** the usual processing.
  */

  pjob->ji_flags |= MOM_CHECKPOINT_POST;

  /*
  ** Set the TI_FLAGS_CHECKPOINT flag for each task that
  ** was checkpointed and aborted.
  */

  strcpy(path,path_checkpoint);
  strcat(path,pjob->ji_qs.ji_fileprefix);
  strcat(path,JOB_CHECKPOINT_SUFFIX);

  dir = opendir(path);

  if (dir == NULL)
    {
    return;
    }

  while ((pdir = readdir(dir)) != NULL) 
    {
    if (pdir->d_name[0] == '.')
      continue;

    tid = atoi(pdir->d_name);

    if (tid == 0)
      continue;

    ptask = task_find(pjob,tid);

    if (ptask == NULL)
      continue;

    ptask->ti_flags |= TI_FLAGS_CHECKPOINT;
    }

  closedir(dir);

  return;
  }






/*
 * start_checkpoint - start a checkpoint going
 *
 *	checkpoint done from a child because it takes a while 
 */

int start_checkpoint(

  job *pjob,
  int  abort,
  struct batch_request *preq)	/* may be null */

  {
#if 0
  static char id[] = "start_checkpoint";
  pid_t     pid;
#endif
  int       rc;
  char      name_buffer[1024];

  if (mom_does_checkpoint() == 0) 	/* no checkpoint, reject request */
    {
    return(PBSE_NOSUP);
    }

  /* Build the name of the checkpoint file before forking to the child because
   * we want this name to persist and this won't work if we are the child.
   * Notice that the ATR_VFLAG_SEND bit causes this to also go to the pbs_server.
   */

  sprintf(name_buffer, "ckpt.%s.%d",pjob->ji_qs.ji_jobid,(int)time(0));
  decode_str(&pjob->ji_wattr[(int)JOB_ATR_checkpoint_name],NULL,NULL,name_buffer);
  pjob->ji_wattr[(int)JOB_ATR_checkpoint_name].at_flags =
    ATR_VFLAG_SET | ATR_VFLAG_MODIFY | ATR_VFLAG_SEND;


  if (!(pjob->ji_wattr[(int)JOB_ATR_checkpoint_dir].at_flags & ATR_VFLAG_SET))
    {
    /* No dir specified, use the default job checkpoint directory /var/spool/torque/checkpoint/42.host.domain.CK */

    strcpy(name_buffer,path_checkpoint);
    strcat(name_buffer,pjob->ji_qs.ji_fileprefix);
    strcat(name_buffer,JOB_CHECKPOINT_SUFFIX);
    decode_str(&pjob->ji_wattr[(int)JOB_ATR_checkpoint_dir],NULL,NULL,name_buffer);
    }


  /* now set up as child of MOM */
#if 0
  pid = fork_me((preq == NULL) ? -1 : preq->rq_conn);

  if (pid > 0) 
    {
    /* parent */

    /* MOM_CHECKPOINT_ACTIVE prevents scan_for_exiting from triggering obits while job is checkpointing. */

    pjob->ji_flags |= MOM_CHECKPOINT_ACTIVE;
    pjob->ji_momsubt = pid; /* record pid in job for when child terminates */

    /* Save the address of a routine to execute once the checkpoint
     * operation is complete.
     *
     * How can this work for job recover when pbs_mom is restarted
     * with a new software load?  There is slight chance that
     * the routine will be in the same place and the result
     * will be a crash.
     */

    pjob->ji_mompost = (int (*)())post_checkpoint; /* BAD routine can move on restart */

    if (preq)
      free_br(preq); /* child will send reply */

    } 
  else if (pid < 0) 
    {
    /* error on fork */

    log_err(errno,id,"cannot fork child process for checkpoint");
    return(PBSE_SYSTEM);	
    } 
  else
#endif
    {
    /* child - does the checkpoint */
    int   admin = 1;
 /* There is no way to tell if admin, server always sends name as PBS_Server
  * and the permissions are not sent in the message and are set to all priv
  * by the mom.
  */

    if ((rc = mom_checkpoint_job(pjob,abort,admin)) == 0)
      {
      /* Normally, this is an empty routine and does nothing. */

      rc = site_mom_postchk(pjob,abort);
      }

    if (preq != NULL) 
      {
      /* rc may be 0, req_reject is used to pass auxcode */

      req_reject(rc,PBS_CHECKPOINT_MIGRATE,preq,NULL,NULL); /* BAD reject is used to send OK??? */
      }
#if 0
    exit(rc);	/* zero exit tells main checkpoint ok */
#endif
    }

  return(PBSE_NONE);		/* parent return */
  }  /* END start_checkpoint() */


/*
 * replace a given file descriptor with the new file path
 *
 * This routine exits on error!  Only used by the BLCR restart code, and
 * there's really no good way to recover from an error in restart.
 */
static int fdreopen(const char *path, const char mode, int fd)
{
  int newfd, dupfd;
 
  close(fd);
 
  newfd = open("/dev/null", O_RDONLY);
  if (newfd < 0)
  {
    perror("open");
    exit(-1);
  }
 
  dupfd = dup2(newfd, fd);
  if (newfd < 0)
  {
    perror("dup2");
    exit(-1);
  }

  close(newfd);

  return dupfd;
}


/*
**	Restart each task which has exited and has TI_FLAGS_CHECKPOINT turned on.
**	If all tasks have been restarted, turn off MOM_CHECKPOINT_POST.
*/

void checkpoint_partial(

  job *pjob)

  {
  static char	id[] = "checkpoint_partial";
  char		namebuf[MAXPATHLEN];
  task		*ptask;
  int		texit = 0;
  extern	char	task_fmt[];

  assert(pjob != NULL);

  strcpy(namebuf,path_checkpoint);
  strcat(namebuf,pjob->ji_qs.ji_fileprefix);
  strcat(namebuf,JOB_CHECKPOINT_SUFFIX);

  for (ptask = (task *)GET_NEXT(pjob->ji_tasks);
       ptask != NULL;
       ptask = (task *)GET_NEXT(ptask->ti_jobtask)) 
    {
    /*
    ** See if the task was marked as one of those that did
    ** actually checkpoint.
    */

    if ((ptask->ti_flags & TI_FLAGS_CHECKPOINT) == 0)
      continue;

    texit++;

    /*
    ** Now see if it was reaped.  We don't want to
    ** fool with it until we see it die.
    */

    if (ptask->ti_qs.ti_status != TI_STATE_EXITED)
      continue;

    texit--;

    if (mach_restart(ptask,namebuf) == -1)
      {
      pjob->ji_flags &= ~MOM_CHECKPOINT_POST;
      kill_job(pjob,SIGKILL,id,"failed to restart");
      return;
      }

    ptask->ti_qs.ti_status = TI_STATE_RUNNING;
    ptask->ti_flags &= ~TI_FLAGS_CHECKPOINT;

    task_save(ptask);
    }

  if (texit == 0) 
    {
    char        oldname[MAXPATHLEN];
    struct stat	statbuf;

    /*
    ** All tasks should now be running.
    ** Turn off MOM_CHECKPOINT_POST flag so job is back to where
    ** it was before the bad checkpoint attempt.
    */

    pjob->ji_flags &= ~MOM_CHECKPOINT_POST;

    /*
    ** Get rid of incomplete checkpoint directory and
    ** move old checkpoint dir back to regular if it exists.
    */

    remtree(namebuf);
    strcpy(oldname,namebuf);
    strcat(oldname,".old");
    if (stat(oldname,&statbuf) == 0) 
      {
      if (rename(oldname,namebuf) == -1)
        pjob->ji_qs.ji_svrflags &= ~JOB_SVFLG_CHECKPOINT_FILE;
      }
    }
  }  /* END checkpoint_partial() */












/* BLCR version of restart */

int blcr_restart_job(
  job  *pjob,  /* I */
  char *file)  /* I */
  {
  char	*id = "blcr_restart_job";
  int   pid;
  char  sid[20];
  char  *arg[20];
  extern  char    restart_script_name[1024];
  task *ptask;
  char  buf[1024];
  char  **ap;

#define SET_ARG(x) (((x) == NULL) || (*(x) == 0))?"-":(x)

  /* if a restart script is defined launch it */

  if (restart_script_name[0] == '\0')
    {
    log_err(-1,id,"No restart script defined");
    }
  else
    {
    /* BLCR is not for parallel jobs, there can only be one task in the job. */
    ptask = (task *) GET_NEXT(pjob->ji_tasks);
    if (ptask != NULL)
      {
 
      /* launch the script and return success */

      pid = fork();
      if (pid < 0)
        {
        /* fork failed */
        return(FAILURE);
        }
      else if (pid == 0)
        {
        /* child: execv the script */

        sprintf(sid,"%ld",
          ptask->ti_job->ji_wattr[(int)JOB_ATR_session_id].at_val.at_long);

        arg[0] = restart_script_name;
        arg[1] = sid;
        arg[2] = SET_ARG(ptask->ti_job->ji_qs.ji_jobid);
        arg[3] = SET_ARG(ptask->ti_job->ji_wattr[(int)JOB_ATR_euser].at_val.at_str);
        arg[4] = SET_ARG(ptask->ti_job->ji_wattr[(int)JOB_ATR_checkpoint_dir].at_val.at_str);
        arg[5] = SET_ARG(ptask->ti_job->ji_wattr[(int)JOB_ATR_checkpoint_name].at_val.at_str);
        arg[6] = NULL;
 
        strcpy(buf,"restart args:");

        for (ap = arg;*ap;ap++)
          {
          strcat(buf," ");
          strcat(buf,*ap);
          }

        log_err(-1,id,buf);

        log_close(0);

        if (lockfds >= 0)
          {
          close(lockfds);
          lockfds = -1;
          }

        net_close(-1);
 
        fdreopen("/dev/null",O_RDONLY,0);
        fdreopen("/dev/null",O_WRONLY,1);
        fdreopen("/dev/null",O_WRONLY,2);

        /* set us up with a new session */

        pid = setsid();

        if (pid < 0)
          {
          perror("setsid");
          exit(-1);
          }

        execv(arg[0],arg);

        return(SUCCESS);  /* Not Reached -- just make the compiler happy */
        }  /* END if (pid == 0) */
      else if (pid > 0)
        {
        /* parent */

        ptask->ti_qs.ti_sid = pid;  /* Apparently torque doesn't do anything with the session ID that we pass back here... */
        ptask->ti_qs.ti_status = TI_STATE_RUNNING;
        task_save(ptask);

#if 0
        /* This does not work, waitpid blocks and the pbs_server times out. */
        int child_status;

        waitpid(pid,&child_status,0);
        if (child_status == 0)
          {
          sprintf(log_buffer,"Restarted 1 task" );

          LOG_EVENT(
            PBSEVENT_JOB,
            PBS_EVENTCLASS_JOB,
            pjob->ji_qs.ji_jobid,
            log_buffer);

          return(SUCCESS);
          }
#else
        return(SUCCESS);
#endif
        }
      }
    }
    return(FAILURE);
  }





/* start each task based on task checkpoint records located job-specific checkpoint directory */

int mom_restart_job(

  job  *pjob,  /* I */
  char *path)  /* I */

  {
  static char	id[] = "mom_restart_job";
  int		i;
  char		namebuf[MAXPATHLEN];
  char		*filnam;
  DIR		*dir;
  struct	dirent	*pdir;
  tm_task_id	taskid;
  task         *ptask;
  int		tcount = 0;
  long		mach_restart A_((task *, char *path));

  if ((dir = opendir(path)) == NULL) 
    {
    sprintf(log_buffer,"opendir %s", 
      path);
 
    log_err(errno,id,log_buffer);

    return(-1);
    }

  strcpy(namebuf,path);
  strcat(namebuf,"/");

  i = strlen(namebuf);

  filnam = &namebuf[i];

  while ((pdir = readdir(dir)) != NULL) 
    {
    if (strlen(pdir->d_name) <= 2)
      continue;

    if ((taskid = (tm_task_id)atoi(pdir->d_name)) == 0) 
      {
      sprintf(log_buffer, "%s: garbled filename %s",
        pjob->ji_qs.ji_jobid, 
        pdir->d_name);

      goto fail;
      }

    if ((ptask = task_find(pjob,taskid)) == NULL) 
      {
      sprintf(log_buffer, "%s: task %d not found",
        pjob->ji_qs.ji_jobid, 
        (int)taskid);

      goto fail;
      }

    strcpy(filnam,pdir->d_name);

    if (mach_restart(ptask,namebuf) == -1) 
      {
      sprintf(log_buffer, "%s: task %d failed from file %s",
        pjob->ji_qs.ji_jobid, 
        (int)taskid, 
        namebuf);

      goto fail;
      }

    ptask->ti_qs.ti_status = TI_STATE_RUNNING;

    if (LOGLEVEL >= 6)
      {
      log_record(
        PBSEVENT_ERROR,
        PBS_EVENTCLASS_JOB,
        pjob->ji_qs.ji_jobid,
        "task set to running (mom_restart_job)");
      }
 
    task_save(ptask);

    tcount++;
    }

  closedir(dir);

  sprintf(log_buffer,"Restarted %d tasks",
     tcount);

  LOG_EVENT(
    PBSEVENT_JOB,
    PBS_EVENTCLASS_JOB,
    pjob->ji_qs.ji_jobid,
    log_buffer);

  return(tcount);

fail:

  log_err(errno,id,log_buffer);

  closedir(dir);

  return(-1);
  }  /* END mom_restart_job() */






/**
 * mom_checkpoint_init_job
 *
 * The routine is called from TMomFinalizeJob1 in start_exec.c.
 * This code initializes checkpoint related variables in the job struct.
 * If there is a checkpoint file, the job is restarted from this image.
 *
 * @param pjob Pointer to job structure
 * @param SC Pointer to TMomFinalizeJob1 state code return variable
 * @see TMomFinalizeJob1
 */
int
mom_checkpoint_init_job(job *pjob,int *SC)
  {
  attribute    *pattr;
  int            i;

  char           buf[MAXPATHLEN + 2];
  struct stat    sb;
  int            rc = FAILURE; /* return code */
  time_t         time_now;
  char           *vp;

  /* Is the job to be periodically checkpointed */

  pattr = &pjob->ji_wattr[(int)JOB_ATR_checkpoint];

  if ((pattr->at_flags & ATR_VFLAG_SET) &&
      ((vp = csv_find_value(pattr->at_val.at_str, "c")) ||
       (vp = csv_find_value(pattr->at_val.at_str, "interval"))))
    {
    /* has checkpoint time (in minutes), convert to milliseconds */

    pjob->ji_checkpoint_time = atoi(vp) * 60;
    pjob->ji_checkpoint_next = pjob->ji_checkpoint_time;
    }

  /* If job has been checkpointed, restart from the checkpoint image */

  if (pjob->ji_wattr[(int)JOB_ATR_checkpoint_dir].at_flags & ATR_VFLAG_SET)
    {
    /* The job has a checkpoint directory specified, use it. */
    strcpy(buf,pjob->ji_wattr[(int)JOB_ATR_checkpoint_dir].at_val.at_str);
    }
  else
    {
    /* Otherwise, use the default job checkpoint directory /var/spool/torque/checkpoint/42.host.domain.CK */
    strcpy(buf,path_checkpoint);
    strcat(buf,pjob->ji_qs.ji_fileprefix);
    strcat(buf,JOB_CHECKPOINT_SUFFIX);
    }

  if (((pjob->ji_qs.ji_svrflags & JOB_SVFLG_CHECKPOINT_FILE) || 
         (pjob->ji_qs.ji_svrflags & JOB_SVFLG_CHECKPOINT_MIGRATEABLE)) &&
       (stat(buf,&sb) == 0)) /* stat(buf) tests if the checkpoint directory exists */
    {
    /* Checkpointed - restart from checkpoint file */

    /* perform any site required setup before restart, normally empty and does nothing */

    if ((i = site_mom_prerst(pjob)) != 0) 
      {
      pjob->ji_qs.ji_un.ji_momt.ji_exitstat = i;

      pjob->ji_qs.ji_substate = JOB_SUBSTATE_EXITING;

      exiting_tasks = 1;

      sprintf(log_buffer,"Pre-restart failed %d",
        errno);

      LOG_EVENT(
        PBSEVENT_JOB,
        PBS_EVENTCLASS_JOB,
        pjob->ji_qs.ji_jobid,
        log_buffer);
      
      return(FAILURE);
      }

    /* For right now, we assume BLCR checkpoint if job has name of checkpoint
     * file.  This file name would have been set at the point where the
     * checkpoint was taken, in the machine dependent checkpoint code.
     *
     * Note: There is a little discrepancy in that the checkpoint work is
     * done at the machine dependent level and the restart is here at a
     * global level.  We hope to correct this soon.
     *
     * The idea is that BLCR itself is architecture independent and so
     * it is okay to invoke it from the main level.
     */

    if (pjob->ji_wattr[(int)JOB_ATR_checkpoint_name].at_flags & ATR_VFLAG_SET)
      {
        rc = blcr_restart_job(pjob,buf);
        pjob->ji_qs.ji_substate = JOB_SUBSTATE_RUNNING;
      }
    else if ((i = mom_restart_job(pjob,buf)) > 0) /* Iterate over files in checkpoint dir, restarting all files found. */
      {
      /* reset mtime so walltime will not include held time */
      /* update to time now minus the time already used    */
      /* unless it is suspended, see request.c/req_signal() */

      time_now = time(0);

      if ((pjob->ji_qs.ji_svrflags & JOB_SVFLG_Suspend) == 0) 
        {
        pjob->ji_qs.ji_stime = 
          time_now - (sb.st_mtime - pjob->ji_qs.ji_stime);

        pjob->ji_qs.ji_substate = JOB_SUBSTATE_RUNNING;

        if (mom_get_sample() != PBSE_NONE)
          mom_set_use(pjob);
        } 
      else 
        {
        pjob->ji_qs.ji_substate = JOB_SUBSTATE_SUSPEND;
        }
      }

    if (rc == FAILURE) 
      {
      /* FAILURE */
  
      /* retry for any kind of changable thing */

      if ((errno == EAGAIN) ||

#ifdef	ERFLOCK
          (errno == ERFLOCK) ||
#endif
#ifdef	EQUSR
          (errno == EQUSR) ||
#endif
#ifdef	EQGRP
          (errno == EQGRP) ||
#endif
#ifdef	EQACT
          (errno == EQACT) ||
#endif
#ifdef	ENOSDS
          (errno == ENOSDS) ||
#endif
          (errno == ENOMEM) ||
          (errno == ENOLCK) ||
          (errno == ENOSPC) ||
          (errno == ENFILE) ||
          (errno == EDEADLK) ||
          (errno == EBUSY))
        {
        pjob->ji_qs.ji_un.ji_momt.ji_exitstat = JOB_EXEC_RETRY;
        }
      else 
        {
        pjob->ji_qs.ji_un.ji_momt.ji_exitstat = JOB_EXEC_BADRESRT;
        }

      pjob->ji_qs.ji_substate = JOB_SUBSTATE_EXITING;

      exiting_tasks = 1;

      sprintf(log_buffer,"Restart failed, error %d",
        errno);

      LOG_EVENT(
        PBSEVENT_JOB,
        PBS_EVENTCLASS_JOB,
        pjob->ji_qs.ji_jobid,
        log_buffer);
      }  /* END else (mom_restart_job() == SUCCESS) */

    /* NOTE:  successful checkpoint handling routes through here */

    return(SUCCESS);
    }  /* END (((pjob->ji_qs.ji_svrflags & JOB_SVFLG_CHECKPOINT_FILE) || ...) */

    return(rc);
  }


