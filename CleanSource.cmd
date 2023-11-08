@echo off
@echo Cleaning all folders
@echo .

rmdir /q /s .vs
rmdir /q /s bin
rmdir /q /s lib
rmdir /q /s BaseLibrary\x64
rmdir /q /s BaseLibrary\Debug
rmdir /q /s BaseLibrary\Release
rmdir /q /s QL\x64
rmdir /q /s QL\Debug
rmdir /q /s QL\Release
rmdir /q /s QLLanguage\x64
rmdir /q /s QLLanguage\Debug
rmdir /q /s QLLanguage\Release
rmdir /q /s SQLComponents\x64
rmdir /q /s SQLComponents\Debug
rmdir /q /s SQLComponents\Release
rmdir /q /s UnitTests\x64
rmdir /q /s UnitTests\Debug
rmdir /q /s UnitTests\Release
rmdir /q /s Test\*.qob

echo .
echo Cleaning is ready
