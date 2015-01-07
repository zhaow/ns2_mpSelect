#创建用于多路径传输的网络拓扑
set ns [new Simulator]

#color
$ns color 1 Red
$ns color 2 Blue

#Use NAM
set nf [open net2.nam w]
$ns namtrace-all $nf

#exec nam net2.nam

#host node
set h1 [$ns node]
set h2 [$ns node]
#router node
set ra1 [$ns node]
set ra2 [$ns node]
set rb1 [$ns node]
set rb2 [$ns node]
#set label
#$h1 label "H1"
#$h1 label-at up
#$h2 label "H2"
#$h2 label-at up

#link DropTail(FIFO)
#host1
$ns duplex-link $h1 $ra1 100Mb 10ms DropTail
$ns duplex-link $h1 $rb1 100Mb 10ms DropTail
#host2
$ns duplex-link $h2 $ra2 100Mb 10ms DropTail
$ns duplex-link $h2 $rb2 100Mb 10ms DropTail
#link
$ns duplex-link $ra1 $ra2 20Mb 10ms DropTail
$ns duplex-link $rb1 $rb2 10Mb 10ms DropTail

#set position
$ns duplex-link-op $h1 $ra1 orient right-up
$ns duplex-link-op $h1 $rb1 orient right-down
$ns duplex-link-op $h2 $ra2 orient left-up
$ns duplex-link-op $h2 $rb2 orient left-down
$ns duplex-link-op $ra1 $ra2 orient right
$ns duplex-link-op $rb1 $rb2 orient right

#set UDP agent
set udp_send [new Agent/UDP]
$ns attach-agent $h1 $udp_send
set udp_recv [new Agent/Null]
$ns attach-agent $h2 $udp_recv
$ns connect $udp_send $udp_recv
$udp_send set fid_ 1

#set CBR app
set cbr [new Application/Traffic/CBR]
$cbr attach-agent $udp_send
$cbr set type_ CBR
$cbr set packet_size_ 1000
$cbr set rate_ 1mb
$cbr set random_ false

#set TCP agnet
set tcp_send [new Agent/TCP]
$tcp_send set calss_ 2
$ns attach-agent $h1 $tcp_send
set tcp_recv [new Agent/TCPSink]
$ns attach-agent $h2 $tcp_recv
$ns connect $tcp_send $tcp_recv
$tcp_send set fid_ 2

#set FTP
set ftp [new Application/FTP]
$ftp attach-agent $tcp_send
$ftp set type_ FTP

#Schedule events
$ns at 0.1 "$cbr start"
$ns at 1.0 "$ftp start"
$ns at 4.0 "$ftp stop"
$ns at 5.0 "$cbr stop"

$ns run
