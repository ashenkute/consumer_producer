# consumer_producer
This program uses locks and semaphores (Pthread API) to implement the cosumer-producer problem.
  The producer will write items into the producer buffer.  
  The middleman will take items from the producer 
  buffer and place them in the consumer buffer.  
  The consumer will read items out of the consumer buffer.
  So, the producer and the middleman share the producer buffer, and the middleman and the consumer share the consumer buffer.
  Command line expects 4 arguments:
