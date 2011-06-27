/**
 * vim:ft=c:fenc=UTF-8:ts=2:sts=2:sw=2:expandtab
 *
 * Copyright 2011 TechnoGate <contact@technogate.fr>
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * This program. If not, see http://www.gnu.org/licenses/
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

int main(int argc, char **argv)
{
  int    file_owner, status;
  char   *newenviron[] = { NULL };
  uid_t  current_uid;
  pid_t  child_pid, wpid;
  ushort file_mode;
  struct stat status_buf;
  struct passwd * passent;

  // Check that we are root!
  current_uid = geteuid();
  if(current_uid != 0) {
    fprintf(stderr, "The wrapper should be ran as root, you should maybe set the owner to root and set setuid.\n");
    return 1;
  }

  // Check argc
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <script> [arguments]\n", argv[0]);
    return 1;
  }

  // Make sure the script does exist
  if(fopen(argv[1], "r") == NULL) {
    fprintf(stderr, "The file %s does not exist.\n", argv[1]);
    return 1;
  }

  // Get the attributes of the file
  if(stat(argv[1], &status_buf) < 0) {
    fprintf(stderr, "Can't stat %s\n", argv[1]);
    return 1;
  }

  // Get the permissions of the file
  file_mode = status_buf.st_mode;

  // Make sure it's executable and it's setuid
  if( !( (file_mode & S_ISUID) || (file_mode & S_IXUSR) ) ) {
    fprintf(stderr, "The file %s should be executable and should have setuid set.\n", argv[1]);
    return 1;
  }

  // Get the owner of the script
  file_owner = status_buf.st_uid;

  // Get the name of the owner
  passent = getpwuid(file_owner);

  // setuid
  if(setuid(file_owner)) {
    if(passent != NULL)
      fprintf(stderr, "Can't change the persona to %s\n", passent -> pw_name);
    else
      fprintf(stderr, "Can't change the persona to %d\n", file_owner);
    return 1;
  }

  // Fork
  child_pid = fork();
  if(child_pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  // Execute the command in a child
  if(child_pid == 0) {
    execve(argv[1], argv + 1, newenviron);
    perror("Failed to execute the script");
    exit(EXIT_FAILURE);
  } else {
    do {
      wpid = waitpid(child_pid, &status, WUNTRACED
    #ifdef WCONTINUED       /* Not all implementations support this */
        | WCONTINUED
    #endif
        );

      // Seems the child is not running
      if (wpid == -1) {
        perror("waitpid");
        exit(EXIT_FAILURE);
      }

      if (WIFEXITED(status)) { /* Child exited ? */
        return WEXITSTATUS(status);


      } else if (WIFSIGNALED(status)) { /* Child signaled ? */
        return WTERMSIG(status);

      } else if (WIFSTOPPED(status)) { /* Child stopped */
        return WSTOPSIG(status);


    #ifdef WIFCONTINUED     /* Not all implementations support this */
      } else if (WIFCONTINUED(status)) {
        // The child is still running...
    #endif
      } else {    /* Non-standard case -- may never happen */
        fprintf(stderr, "Unexpected status (0x%x)\n", status);
      }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  // We should never reach this code.
  fprintf(stderr, "Unexpected behaviour, thread was not running and no error were raised.\n");
  return EXIT_FAILURE;
}