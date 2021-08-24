# Compile
  command: sh script
# How to upload files to server
  If you want to upload files from your device to server, you can use following command.  
  &emsp;&emsp;command: scp upload_files cadb0065@140.110.214.97:/destination_path  
  &emsp;&emsp;ex: scp main.cpp cadb0065@140.110.214.97:~/Desktop/  
  &emsp;&emsp;ex: scp -r someDirectory/ cadb0065@140.110.214.97:\~/User_dir
# Run exe
  .\exe_name lef_name.lef def_name.def txt_name.txt desired_output_file_name.def  
  order doesn't matter  
  ex:  
  .\cadb0065 case1/caseSample.lef case1/caseSample.def case1/caseSample.txt outSample.def

# update_log parallel,cs_double,four_improve,check_invalid
  parallel: Divide LP model into two models with x, y direction respectly. Use pthread for parallel(add -pthread into script).  
  cs_double: Solve the bug of converting type between 'int' and 'double'.  
  four_improve: Introduce four kinds of improve strategies.  
  check_invalid: In previous version(beta sbmission), we forgot to update invalid macros during SA processing. The bug now solved.
