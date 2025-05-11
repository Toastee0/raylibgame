Get-ChildItem -Path . -Recurse -Include *.c, *.cpp, *.h, *.cs, *.js, *.ts, *.py, *.java, *.rb, *.go, *.php, *.html, *.css, *.json, *.xml | 
Select-Object -ExpandProperty FullName | 
Out-File -FilePath .\source_file_list.txt -Encoding utf8
