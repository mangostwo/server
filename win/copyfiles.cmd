echo ******************************************************
echo ** COPY STEP                                        **
echo ******************************************************
echo .
echo **********************
echo ** Source Path      **
echo **********************
echo Source: %1
echo .
echo **********************
echo ** Destination Path **
echo **********************
echo Destination: %2
echo .
echo **********************
echo ** Current Path     **
echo **********************
cd
echo .
echo **********************
echo ** DLL's            **
echo **********************
copy ..\%1\*.dll %2
echo **********************
echo ** mangosd.conf     **
echo **********************
copy ..\..\src\mangosd\mangosd.conf.dist.in %2\mangosd.conf.dist
echo **********************
echo ** ahbot.conf       **
echo **********************
copy ..\..\src\game\AuctionHouseBot\ahbot.conf.dist.in %2\ahbot.conf.dist
echo .
echo **********************
echo * Eluna base Scripts *
echo **********************
XCOPY "..\..\src\game\LuaEngine\extensions" "%2\lua_scripts\extensions" /E /I /F /Y