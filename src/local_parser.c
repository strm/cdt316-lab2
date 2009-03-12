int what(void){
#if LOCAL_PARSE
	/*
	 * Testing local parser
	 */
	printf("Welcome to local parser\n");
	//socket settings
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);
	int psock;
	int res;
	long num_cmds;
	//transaction
	transNode * trans = createTransaction(0); //TODO add uniq id here
	//open socked to client
	psock = accept(sock, (struct sockaddr *)&addr, &addrlen);
	if(!psock){
		debug_out(2, "Failed to accept incoming connection.\n");
		exit(1);
	}
	printf("Accepted client connection maybe\n");
	//read number of commands
	res = force_read(psock, &num_cmds, sizeof(num_cmds));
	if( res < sizeof(num_cmds) ){
		if( res < 0) 
			my_perror(2, "force_read()");
		else
			debug_out(2, "Failed to read number of lines (%d)\n");
	}
	num_cmds = ntohl(num_cmds);
	num_rsp = 0;
	//Read all commands
	for (; num_cmds; num_cmds--){
		 command cmd;
		 response rsp;
		 memset(&rsp, 0, sizeof(rsp));
		 //read the command
		 res = force_read(psock, &cmd, sizeof(command));
		 if(res < sizeof(command)){
		 	if( res  < 0 )
				my_perror(2, "force_read");
			else
				debug_out(2, "Failed to read command (%d)\n");
			exit(1);
		 }

		 cmd.op = ntohl(cmd.op);
		 rsp.seq = cmd.seq;
		 /*
		  * Add to list of unparsed commands here
		  */
		 if(!varListPush(cmd, &(trans->unparsed))){
			 debug_out(5, "Couldn't add command struct to varList\n");
			 exit(2);
		 }
		 //TODO DEBUG EXTRA TODO
		 if(!varListFind(cmd.arg1, (trans->unparsed))){
		 	debug_out(5, "Command struct not found in list\n");
			exit(2);
		 }

	}
	//TODO CLOSE PSOCK FOR TESTING ONLY TODO
	close(psock);
	//parse the list of commands
	printf("GET LIST OF USED VARIABLES\n");
	if( getUsedVariables(&(trans->parsed), trans->unparsed) == 0){
		debug_out(5, "Failed to create a list of variables from the unparsed list\n");
		exit(2);
	}
	//Do local parsing
	printf("LOCALPARSE\n");
	if( !localParse(&(trans->parsed), trans->unparsed) ){
		debug_out(5, "localParse Failed\n");
		exit(2);
	}
	//print the list
	printf("PRINY LOCALLY PARSED VALUES\n");
	varList * iter = trans->parsed;
	while ( iter != NULL ){
		printf("{%s: %s }", iter->data.arg1, iter->data.arg2);
		iter = iter->next;
	}
	printf("\nThats it\n");
	/*
	 * End parser tests.
	 */
#endif
	pthread_join(listenThread, NULL);
	return 0;
}
