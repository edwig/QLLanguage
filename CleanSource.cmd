@echo off
@echo Opschonen directories
@echo .

rmdir /q /s Debug
rmdir /q /s Release
rmdir /q /s BaseLibrary\x64
rmdir /q /s BaseLibrary\Debug
rmdir /q /s BaseLibrary\Release
rmdir /q /s SQLComponents\x64
rmdir /q /s SQLComponents\Debug
rmdir /q /s SQLComponents\Release
rmdir /q /s UnitTests\x64
rmdir /q /s UnitTests\Debug
rmdir /q /s UnitTests\Release
rmdir /q /s QL\x64
rmdir /q /s QL\Debug
rmdir /q /s QL\Release
rmdir /q /s Test\*.qob
rmdir /q /s x64
rmdir /q /s bin
rmdir /q /s lib
rmdir /q /s .vs

pause