@echo off
md "bin" >nul 2>&1
copy source bin >nul 2>&1 
copy example bin >nul >nul 2>&1

make

del "bin\*" /f /q /s >nul 2>&1
rmdir "bin" >nul 2>&1