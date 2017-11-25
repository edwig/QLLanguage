@echo off
@echo Opschonen directories
@echo .

del "QL++Language2.rar"
del /q /s /f *.sdf
rmdir /q /s ipch
rmdir /q /s Debug
rmdir /q /s Release
rmdir /q /s x64

echo .
echo Klaar met opschonen
echo .
echo maak een winrar bestand aan
echo .
"C:\Program Files\Winrar\rar.exe" a "QL++Language2.rar" *.cpp *.h *.sln *.vcxproj *.filters *.rc *.txt *.bat *.ql *.qob SQLComponents\*.*
echo .
echo Klaar met RAR aanmaken
echo .
copy "QL++Language2.rar" C:\Users\%USERNAME%\OneDrive\Documenten\Language
echo .
echo Klaar met archief naar de cloud kopieren
pause