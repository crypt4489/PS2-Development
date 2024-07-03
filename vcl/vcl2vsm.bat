echo off
set arg1=%~nx1
set arg2=%arg1:~0,-4%
set outputFolder=.\\output\\
vcl_14beta7.exe -o%outputFolder%%arg2%.vsm %arg1%
pause