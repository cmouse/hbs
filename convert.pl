#!/usr/bin/perl

## converts eggdrop user or channel file to HBS bot files
use warnings;
use strict;
use Getopt::Long;
use Data::Dumper;

my $user = 0;
my $channel = 0;
my $ifile = "";
my $ofile = "";

sub print_user {
    my ($user) = @_;
    print OUTPUT "\t<user>\n";
    print OUTPUT "\t\t<name><![CDATA[".$user->{'user'}."]]></name>\n";
    if (defined($user->{'admin'})) {
	print OUTPUT "\t\t<admin>1</admin>\n";
    }
    if (defined($user->{'laston'})) {
	print OUTPUT "\t\t<laston>".$user->{'laston'}."</laston>\n";
    }
    print OUTPUT "\t\t<channels>\n";
    for my $channel (sort keys %{$user->{'channels'}}) {
	my $modes = $user->{'channels'}->{$channel}->{'modes'};
	my $mode = 0;
	if ($modes =~ /n/) {
	    $mode = 28;
	} elsif ($modes =~ /m/) {
	    $mode = 14;
	} elsif ($modes =~ /o/) {
	    $mode = 4;
	} elsif ($modes =~ /v/) {
	    $mode = 2;
	} elsif ($modes =~ /k/) {
	    $mode = 1;
	}
	print OUTPUT "\t\t\t<channel flags=\"$mode\"><![CDATA[$channel]]></channel>\n";
    }
    print OUTPUT "\t\t</channels>\n";
    print OUTPUT "\t</user>\n";
}

sub parse_user {
    # user open
    my $user;
    my $uo;
    print OUTPUT "<users>\n";
    while(<INPUT>) {
	chomp;
	if (/--LASTON (\d+)/) {
	    # this is a special line like password	    
	    $user->{'laston'} = $1;
	} elsif (/!\s+([\#.-_&+\w]+)\s+\d+\s+(\w+)/) {
	    # this is a channel mode line
	    $user->{'channels'}->{$1}->{'modes'} = $2;
	} elsif (/([\w][\S]*)\s+- (\w+)/) {
	    # this is a user definition line
	    if ($uo) {
		 print_user $user;
	    }
	    $user = {'user' => $1};
	    $user->{'admin'} = 1 if ($2=~/n/);
	    $uo = 1;
	}
    }
    if ($uo) {
	print_user $user;
    }
    print OUTPUT "</users>\n";
}

sub parse_channel {
    print OUTPUT "<channels>\n";
    while(<INPUT>) {
	chomp;
	s/\s+/ /g;
	if (/channel add (\S+) { (.*?) }/) {
	    my @vals = split /([{].*?[}]|\s+)/,$2;
	    my $i = 0;
	    print OUTPUT "\t<channel>\n";
	    print OUTPUT "\t\t<name><![CDATA[".$1."]]></name>\n";
	    while($i < @vals) {
		my $val = $vals[$i];
		if ($val=~/^\s+$|^$/) {
		    $i++;
		    next;
		}
		# process flag
		if ($val eq "chanmode") {
		    $i++;
		    while(($i<@vals)&&($vals[$i]=~/^\s+$|^$/)) {
			$i++;
		    }
		    if ($vals[$i] =~ /^{(\S+)\s+(\S+?)}$/){
			my $mod = $1;
			my $key = $2;
			if ($1 =~ /k/) {
			    print OUTPUT "\t\t<modes>".$mod." ".$key."</modes>\n";
			} 
		    } else {
			print OUTPUT "\t\t<modes>".$vals[$i]."</modes>\n";
		    }
		} elsif ($val eq "udef-int-firstjoin") {
		    $i++;
		    while(($i<@vals)&&($vals[$i]=~/^\s+$|^$/)) {
                        $i++;
                    }
		    print OUTPUT "\t\t<created>".$vals[$i]."</created>\n";
		} elsif ($val eq "udef-int-lastjoin") {
		    $i++;
		    while(($i<@vals)&&($vals[$i]=~/^\s+$|^$/)) {
                        $i++;
                    }
                    print OUTPUT "\t\t<lastjoin>".$vals[$i]."</lastjoin>\n";
		} elsif ($val eq "udef-int-lastop") {
		    $i++;
		    while(($i<@vals)&&($vals[$i]=~/^\s+$|^$/)) {
                        $i++;
                    }
                    print OUTPUT "\t\t<lastop>".$vals[$i]."</lastop>\n";
		}
		$i++;
	    }
	    print OUTPUT "\t</channel>\n";
	}
    }
    print OUTPUT "</channels>\n";
}

sub main {
    if (GetOptions("user" => \$user,
		   "channel" => \$channel,
		   "infile=s" => \$ifile,
		   "outfile=s" => \$ofile) == 0) {
	
	return 1;
    }
    
    if (($user == 0)&&($channel == 0)) {
	print "Invalid syntax\n";
	return 1;
    }
    
    open INPUT,"<".$ifile or die("Unable to open $ifile for reading: $!");
    open OUTPUT,">".$ofile or die("Unable to open $ofile for writing: $!");
    
    if ($user != 0) {
	print "User parser starting on $ifile\n";
	parse_user;
    } else {
	print "Channel parser starting on $ifile\n";
	parse_channel;
    }

    close OUTPUT;
    close INPUT;
}

main;
