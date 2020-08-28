for((i=0;i<15;++i)); do
./chatty -f ./DATA/chatty.conf2 > /dev/null &
./testconf.sh /tmp/chatty_socket_516801 /tmp/chatty_stats.txt > /dev/null
if [[ $? != 0 ]]; then
 echo "TEST FALLITO"
 exit 1
fi
killall -9 chatty
done
echo "TEST OK!"
