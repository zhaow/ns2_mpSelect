#Use hierarchical address
set ns [new Simulator]
$ns set-address-format hierarchical

#Use NAM
set nf [open net3.nam w]
$ns namtrace-all $nf

#set route protol
#$ns rtproto Session

AddrParams set domain_num_ 4
lappend cluster_num 1 2 2 1
AddrParams set cluster_num_ $cluster_num
lappend eilastlevel 2 2 2 2 2 2
AddrParams set nodes_num_ $eilastlevel
               
set naddr {0.0.1 0.0.0 1.0.1 1.0.0 1.1.0 1.1.1 2.0.1 2.0.0 2.1.0 2.1.1 3.0.0 3.0.1}

for {set i 0} {$i < 12} {incr i} {
	set n($i) [$ns node [lindex $naddr $i]]
}

#0.0.0->0.0.1
$ns duplex-link $n(0) $n(1) 100Mb 10ms DropTail

#select1
$ns duplex-link $n(1) $n(2) 100Mb 10ms  DropTail
$ns duplex-link $n(2) $n(3) 100Mb 10ms DropTail
$ns duplex-link $n(1) $n(6) 100Mb 10ms  DropTail
$ns duplex-link $n(6) $n(7) 100Mb 10ms DropTail

#route path AB
$ns duplex-link $n(3) $n(4) 20Mb 10ms DropTail
$ns duplex-link $n(7) $n(8) 10Mb 15ms DropTail

#select2
$ns duplex-link $n(4) $n(5) 100Mb  10ms DropTail
$ns duplex-link $n(5) $n(10) 100Mb 10ms  DropTail
$ns duplex-link $n(8) $n(9) 100Mb  10ms DropTail
$ns duplex-link $n(9) $n(10) 100Mb 10ms  DropTail

#3.0.0->3.0.1
$ns duplex-link $n(10) $n(11) 100Mb 10ms DropTail

$ns duplex-link-op $n(0) $n(1) orient right
$ns duplex-link-op $n(1) $n(2) orient right-up
$ns duplex-link-op $n(2) $n(3) orient right
$ns duplex-link-op $n(1) $n(6) orient right-down
$ns duplex-link-op $n(6) $n(7) orient right
$ns duplex-link-op $n(3) $n(4) orient right
$ns duplex-link-op $n(7) $n(8) orient right
$ns duplex-link-op $n(5) $n(10) orient right-down
$ns duplex-link-op $n(4) $n(5) orient right
$ns duplex-link-op $n(9) $n(10) orient right-up
$ns duplex-link-op $n(8) $n(9) orient right
$ns duplex-link-op $n(10) $n(11) orient right

for {set i 0} {$i < 12} {incr i} {
	puts "node$i is [$n($i) node-addr] [$n($i) entry]"
}

#$n(2) add-route $n(11) $n(3)
#$n(3) add-route $n(11) $n(4)
#$n(4) add-route $n(11) $n(5)
#$n(5) add-route $n(11) $n(10)

#$n(2) split-addrstr 1.2.3

#for myPing
Agent/myPing instproc recv {from rtt} {
	$self instvar node_
	puts "node [$node_ id] received ping answer from $from in $rtt ms."
}

set ping0 [new Agent/myPing]
$ns attach-agent $n(0) $ping0
set ping1 [new Agent/myPing]
$ns attach-agent $n(11) $ping1

set ping2 [new Agent/myPing]
$ns attach-agent $n(2) $ping2
set ping3 [new Agent/myPing]
$ns attach-agent $n(9) $ping3

set ping4 [new Agent/myPing]
$ns attach-agent $n(7) $ping4
set ping5 [new Agent/myPing]
$ns attach-agent $n(5) $ping5

$ns connect $ping0 $ping1
$ns connect $ping2 $ping3
$ns connect $ping4 $ping5

$ns at 0.1 "$ping0 send"
$ns at 0.2 "$ping1 send"
$ns at 0.3 "$ping2 send"
$ns at 0.4 "$ping3 send"
$ns at 0.5 "$ping4 send"
$ns at 0.6 "$ping5 send"

$ns run
