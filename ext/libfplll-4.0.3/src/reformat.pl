#!/usr/bin/perl

$r=@ARGV[0];
$c=@ARGV[1];



$i=0;
while(<stdin>)
{
    while (s/^[^0123456789\-\+]*([\-]?[0-9]+)([^0-9].*)$/$2/)#$1=first number in chain, $2=queue of string -> $_. 
    {
	@v[$i++]=$1;
    }
}

die "bizarre : $i != $r * $c" if ($i!=$r*$c);
print "[";
$k=0;
for ($i=0;$i<$r;$i++)
{
    print "[";
    for ($j=0;$j<$c;$j++)
    {
	print @v[$k++];
	print " ";
    }
    print "]\n";
}
print "]";
