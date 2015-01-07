#创建用于多路径传输的网络拓扑
set ns [new Simulator]

#Use NAM
set nf [open net1.nam w]
$ns namtrace-all $nf

#exec nam net1.nam

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

#$ns at 5.0 "print hello"

$ns run
