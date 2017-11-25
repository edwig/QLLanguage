@echo off
@echo Opschonen  directories
@echo .

del "QL++Language2.rar"
del /q /s /f *.sdf
rmdir /q /s Debug
rmdir /q /s Release
rmdir /q /s ipch
rmdir /q /s x64
echo .
echo Klaar met opschonen
echo .
echo Haal bestand uit de cloud
copy "C:\Users\%USERNAME%\SkyDrive\Documenten\Language\QL++Language2.rar" .
echo .
echo Klaar met RAR kopieren
echo .
"C:\Program Files\Winrar\rar.exe" x -o+ "QL++Language2.rar"
echo .
echo Klaar met archief uit de cloud halen
echo .
pause