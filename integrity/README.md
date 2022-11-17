
show how to check files integrity with C effetively
specially show how to get effective likely using hashmap without implementation if data are fixed amount

enjoy!

You can bench performance simply with uftrace
"""
$ uftrace record --no-libcall -P INTEGRITY_ARRAY -P INTEGRITY_HASHWAY integrity
$ uftrace report
"""

the result you can see
"""
  Total time   Self time       Calls  Function
  ==========  ==========  ==========  ====================
   59.760  s    0.600 us           1  main
   59.760  s    1.890  s           1  INTEGRITY_BENCH
   57.823  s   57.643  s      100000  INTEGRITY_ARRAY
  180.775 ms  180.775 ms         152  linux:schedule (pre-empted)
   45.103 ms   45.057 ms       99999  INTEGRITY_HASHWAY
"""
