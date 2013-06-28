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
copy ..\..\src\mangosd\mangosd.conf.dist.in %2
echo **********************
echo ** ahbot.conf       **
echo **********************
copy ..\..\src\game\AuctionHouseBot\ahbot.conf.dist.in %2
rem echo **********************
rem echo ** mods.conf        **
rem echo **********************
rem copy ..\..\src\game\extras\mods.conf.dist.in %2
