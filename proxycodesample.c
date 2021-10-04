/* 
 * Requires:
 *   The port is an unused TCP port number.
 *
 * Effects:
 *   Establishes multiples connections between client and proxy.
 */
int
main(int argc, char **argv)
{
	int listenfd, errno, i;
	int *connfd;
	FILE *fp;

	struct sockaddr_storage *clientaddrp;
	socklen_t clientlen;
	pthread_t tid;

	/* Check the arguments. */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
		exit(0);
	}

	/* Creates an empty file for logging. */
	fp = fopen("proxy.log", "w"); 

	sbuf = malloc(sizeof(sbuf_t));
	
	/* Open a socket and listen for a connection request. */
	listenfd = open_listenfd(argv[1]);

	/* Initializing sbuf */
	sbuf->shared_buffer = calloc(SBUFSIZE, sizeof(struct data));
	sbuf->cnt = 0;
	sbuf->front = 0;
	sbuf->rear = 0;
	pthread_mutex_init(&(sbuf->mutex), NULL);
	pthread_cond_init(&(sbuf->cond_empty), NULL);
	pthread_cond_init(&(sbuf->cond_full), NULL);
	
	struct data *input = malloc(sizeof(struct data *));

	/* Creating worker threads */
	for (i = 0; i < NTHREADS; i++) {
		pthread_create(&tid, NULL, thread, NULL);
	}

	pthread_cond_signal(&(sbuf->cond_empty));
	signal(SIGPIPE, SIG_IGN);

	while (1) {
		clientlen = sizeof(struct sockaddr_storage);

		/* Accepts a pending connection request. */
		connfd = malloc(sizeof(int));
		clientaddrp = malloc(sizeof(struct sockaddr));
		*connfd = accept(listenfd, (struct sockaddr *)clientaddrp, 
		    &clientlen);

		/* Fill in info. */
		input->connfd = connfd;
		input->fp = fp;
		input->clientaddrp = clientaddrp;

		/* Checks if there is room in sbuf to insert input data. */
		pthread_mutex_lock(&sbuf->mutex);
		printf("producer locked");

		if (sbuf->cnt == SBUFSIZE) {
			printf("waiting");
			pthread_cond_wait(&sbuf->cond_full, &sbuf->mutex);
		}
		/* Update count */
		sbuf->cnt++;

		sbuf->shared_buffer[(++sbuf->rear)%(SBUFSIZE)] = *input;

		if (sbuf->rear == SBUFSIZE - 1)
			sbuf->rear = 0;
		else
			sbuf->rear++;

		pthread_cond_signal(&(sbuf->cond_empty));

		pthread_mutex_unlock(&sbuf->mutex);
		
	}

	/* Terminating thread */
	pthread_join(tid, NULL);

	/* Clean up pthreads */
	pthread_mutex_destroy(&sbuf->mutex);
	pthread_cond_destroy(&sbuf->cond_empty);
	pthread_cond_destroy(&sbuf->cond_full);

	free(sbuf);

	/* Close log and exit main thread */
	fclose(fp); 
	exit(0); 

	/* Return success. */
	return (0);
}
