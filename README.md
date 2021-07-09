# Compile
  command: sh script
# How to upload files to server
  If you want to upload files from your device to server, you can use following command.  
  &emsp;&emsp;command: scp upload_files cadb0065@140.110.214.97:/destination_path  
  &emsp;&emsp;ex: scp main.cpp cadb0065@140.110.214.97:~/Desktop/  
  &emsp;&emsp;ex: scp -r someDirectory/ cadb0065@140.110.214.97:\~/User_dir
# Run exe
  .\exe_name lef_name.lef def_name.def txt_name.txt desired_output_file_name.def
  &emsp;order doesn't matter
  &emsp;ex:
  &emsp;.\cadb0065 case1/caseSample.lef case1/caseSample.def case1/caseSample.txt outSample.def
