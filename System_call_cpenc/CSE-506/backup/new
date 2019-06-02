/* 
 * To check whether the file exists in user land or not
 * Bypass this while checking the system call till parse
 *access() succeeds and shall return 0; otherwise, -1 shall be returned and errno shall be set to indicate the error
 *As discussed in the class, I have implemented the userlevel checks for input and output
 *Especially for the output, I need to check whether the output folder exists or not and then check for the permissions of the output file to write using W_OK
 *
 */
	    if (file_isreg(input)==1)
	        {
	        printf("Input File is Regular.Checking for the access permissions\n"); 
	        if (access(input,R_OK)==0)
			{
			printf("Read access is available for input file \n");
			}
		else
			{
			printf("Read access is not allowed\n");
			retval=-EPERM;
			goto out;	
			}//Closing the if-else block of access_ok
		}
	    else
		{
		printf("File doesn't exists.\n");
	 	goto out;
		}
/*
 *To check if the relative path to the output folder exists or not
 *First part is to generate the output
 */
		if(strchr(output,'/'))
		{   printf("Entering the folder block %s %lu \n", output, strlen(output));
		    long unsigned int fold_ptr; 
	            int l;
		    for(l=strlen(output)-1;l > -1; l--)
	            {
	                    if (output[l]=='/')
		                { fold_ptr=l;
		                    break;
		                }
	            }//Closing the for loop
	            char str[sizeof((fold_ptr+1)*sizeof(char))];
                    //str=malloc(sizeof((fold_ptr+1)*sizeof(char)));
	            strncpy(str, output, fold_ptr);
                    printf("Folder to be checked %s\n",str);
	            if (folder_isreg(str))
	            {
			printf("Output file relative directory exist and continue what you are doing\n");
                	if (access(str,W_OK)==0)
			{
			printf("Write access of the output relative folder is available\n");
			goto parse;
			}
			else
			{
			printf("Permissions to output relative folder is not present\n");
			retval=-EPERM;
			goto out;
			}
		    }//Closing the folder_isreg if block
	            else
	            {   
			printf("Output file relative directory doesn't exist\n");
			goto out;
	            }//Closing the folder_isreg else block
	        }//Closing the strchr if block
	        else
	        {
		   if (file_isreg(output)==1)
		   {
			    if(access(output,W_OK)==0)
			    {
				printf("You can overwrite the existing output file\n");
			    	goto parse;
				}
			    else
			    {
		            	printf("Write access to the output file is not present.\n");
			    	retval =-EPERM;
			    	goto out;
			    }
		  } 
		  else
		  {/*Checking whether you can access the current working directory */
			    if(access("./",W_OK)==0)
			    {
				printf("You can create the output file in the current folder\n");
			    	goto parse;
				}
			    else
			    {
		            	printf("Write access to the output file is not present.\n");
			    	retval =-EPERM;
			    	goto out;
			    }

		   }
	   }//Closing the strchr else block


