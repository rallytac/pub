
sudo tc qdisc add dev ens33 root handle 1: netem delay 100ms 25ms
sudo tc qdisc del dev ens33 root handle 1

sudo tc qdisc add dev ens33 root handle 1: netem delay 100ms 25ms
sudo tc qdisc add dev ens33 root handle 1: netem delay 100ms

sudo tc qdisc change dev ens33 root handle 1: netem delay 100ms 25ms

sudo tc qdisc add dev ens33 root handle 1: netem delay 100ms 25ms
sudo tc qdisc del dev ens33 root handle 1

