@echo off
echo Making HTMs in AGK Txt...
cd Generator
Generator.exe
cd ..
echo Making Reference HTMs in Reference\Output...
cd Reference
mkdir Output
cd Output
.\..\Reference.exe
echo Copying Files...
xcopy *.* ".\..\..\AGK Txt\Reference\" /EXCLUDE:..\ignore.txt /E /Y
copy keywordlinks.txt ".\..\..\AGK Txt\keywordlinks.txt"
echo Done
pause.exe
