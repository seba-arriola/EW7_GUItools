#ifdef SUNOS
#ifdef __STDC__ 
   void exit_handler(int status, int arg);
#else
  void exit_handler();
#endif
#endif


#ifdef SOLARIS
   void exit_handler();
#endif
	void user_shutdown();
