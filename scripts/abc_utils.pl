#!/usr/bin/perl



use Carp;
use strict;
# use Math::VecStat qw(max min maxabs minabs sum average);
# use local::lib "/home/holcomb/Perl_Modules"; #installs modules without sudo

#use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;


sub print_result_table {
     my $R = shift;
     printf ("%6s, %10s, %5s, %6s, %-s\n","frames", "runtime", "valid", "engine", "invariants");
     foreach my $i (0..$#$R) {
	  printf ("%6i, %10.3f, %5i, %6s, %-s\n",
		  $R->[$i]->{FRAMES},
		  $R->[$i]->{RUN_TIME},
		  $R->[$i]->{VALID},
		  $R->[$i]->{ENGINE},
		  $R->[$i]->{INVARIANTS}
		 );
     }
     return;
}


sub parse_abc {
     my ($engine,$out) = @_;

     my $r;
     if ($engine eq "pdr") {
	  $r = parse_pdr($out);
     } elsif ($engine eq "kind") {
	  $r = parse_kind($out);
     } else {
	  croak;
     }
     return $r;
} 

#
# 
#
sub  parse_kind {
     my $out = shift;
     my $k = 0;
     my $r = {
	      "FRAMES" => -1,
	      "VALID" => -1,
	      "RUN_TIME" => -1,
	      "TIMEOUT" => 0
	     };

     if ($out =~ /Completed (\d+) interations[\w\s]*.[\W]*Networks are equivalent.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID} = 1;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /Completed (\d+) interations[\w\s]*.[\W]*Networks are UNDECIDED.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID} = 0;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } else {
	  writeFile("abc.out",Dumper($out,$r));
	  carp "\n\nPossible timeout - cannot understand results";
	  

	  # see how many frames were used?

	  # find the largest frame explored
	  my @lines = split(/[\n\r]/,$out);
	  my $max_frame = -1;
	  foreach my $line (grep {/^\s*\d+\s*:\s*[01x]+$/} @lines) {
	       my $frame = $1 if ($line =~ /\s*(\d+)\s*:.*/);
	       $max_frame = ($max_frame > $frame) ? $max_frame : $frame;
	  }
	  $r->{FRAMES} = $max_frame;



	  $r->{TIMEOUT} = 1;
     }
     
     writeFile("abc.out",Dumper($out,$r));

     # print "\n\n".Dumper($out,$r);
     return $r;
}





#
# parse the dump from a run of PDR
#
sub  parse_pdr {
     my $out = shift;

     my $r = {
	      "FRAMES" => -1,
	      "VALID" => -1,
	      "RUN_TIME" => -1,
	      "TIMEOUT" => 0
	     };

     if ($out =~ /Reached limit on the number of timeframes \((\d+)\).[\w\W]+Property UNDECIDED.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID} = 0;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /Property DISPROVED in frame (\d+) [\w\W]+.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID} = 0;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /Invariant F\[(\d+)\] :[\w\W]+Property proved.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID} = 1;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } else {
	  #print Dumper($out);
	  writeFile("abc.out",Dumper($out,$r));
	  carp "\n\nPossible timeout - cannot understand results";
	  
	  # find the largest frame explored
	  my @lines = split(/[\n\r]/,$out);
	  my $max_frame = 0;
	  foreach my $line (grep {/\s*\d+\s*:[\s\d\.]+[\d\.]+ sec/} @lines) {
	       my $frame = $1 if ($line =~ /\s*(\d+)\s*:.*/);
	       $max_frame = ($max_frame > $frame) ? $max_frame : $frame;
	  }
	  $r->{FRAMES} = $max_frame;
	  $r->{TIMEOUT} = 1;
     }

     writeFile("abc.out",Dumper($out,$r));
     return $r;
}


#
# parse the dump from a run of bmc
#
sub  parse_bmc {
     my $out = shift;

     my $r = {
	      "FRAMES"   => -1,
	      "VALID"    => 'NA',
	      "INVALID"  => -1,
	      "RUN_TIME" => -1,
	      "TIMEOUT"  =>  0
	     };

     if ($out =~ /Output \d+ asserted in frame (\d+) .*. Time\s+=\s+([\d\.]+) sec/) {
	  $r->{INVALID} = 1;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /No output asserted in (\d+) frames..*Time\s+=\s+([\d\.]+) sec/) {
	  $r->{INVALID} = 0;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } else {

	  carp "cannot understand results, maybe is timeout";

	  my @lines = split(/[\n\r]/,$out);
	  my $max_frame = 0;
	  foreach my $line (grep {/\s*\d+\s*\+\s*:.+\s+[\d\.]+ sec/} @lines) {
	       my $frame = $1 if ($line =~ /\s*(\d+)\s*\+\s*:.*/);
	       $max_frame = ($max_frame > $frame) ? $max_frame : $frame;
	  }
	  $r->{INVALID} = 0;
	  $r->{FRAMES} = $max_frame;
	  $r->{TIMEOUT} = 1;
	  #print Dumper($out);
     }


     writeFile("abc.out",Dumper($out,$r));
     return $r;
}















#
# run a single instance of k-induction with ABC, returns the results
#
sub  abc_kind {
     my ($args) = @_;

     my $input_file    = $args->{"INPUT_FILE"};
     my $k             = $args->{"K"};

     print "\nstarting kind: ".Dumper($args);
     
     #writeFile("abc.script","r $input_file; orpos; \nind -F $k -C 100000000 -v;\n");

     #writeFile("abc.script","r $input_file;\n\n prefix;\n\n l2s -l;\n\n ind -F $k -C 100000000 -va;\n");
     #writeFile("abc.script","r $input_file; prefix; l2s -l; ps; bmc -v -F $k; \nind -F $k -C 100000000 -vu;\n");

     # the next is same as at Intel
     #  and they have a note about the importance of BMC (but why?)
     writeFile("abc.script","r $input_file; prefix; l2s -l; ps; bmc -v -F $k; \nind -F $k -C 100000000 -vu;\n");

     #writeFile("abc.script","r $input_file; cycle -x -F 1; \nind -F $k -C 100000000 -vu;\n");

     my $out = `abc -F abc.script`;

     my $r = {
	      "MAX_FRAMES" => $k,
	      "FRAMES" => -1,
	      "VALID" => -1,
	      "RUN_TIME" => -1,
	      "OUT" => $out,
	     };

     if ($out =~ /Completed (\d+) interations[\w\s]*.[\W]*Networks are equivalent.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID} = 1;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /Completed (\d+) interations[\w\s]*.[\W]*Networks are UNDECIDED.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID} = 0;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } else {
	  writeFile("abc.out",Dumper($out,$r));
	  croak "cannot understand results";
     }
     
     writeFile("abc.out",Dumper($out,$r));

     #     print "\n\n".Dumper($out,$r);

     return $r;
}



#
# run a single instance of bmc with ABC, returns the results
#
sub  abc_bmc {
     my ($args) = @_;

     my $input_file    = $args->{"INPUT_FILE"};
     my $f             = $args->{"FRAMES"};

     print "\nstarting bmc: ".Dumper($args);
     
     writeFile("abc.script","r $input_file; \nbmc -F $f -v;\n");
     my $out = `abc -F abc.script`;

     my $r = {
	      "MAX_FRAMES" => $f,
	      "FRAMES" => -1,
	      "VALID" => -1,
	      "RUN_TIME" => -1,
	     };

     if ($out =~ /Output \d+ asserted in frame (\d+) .*. Time\s+=\s+([\d\.]+) sec/) {
	  $r->{VALID} = 0;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /No output asserted in (\d+) frames..*Time\s+=\s+([\d\.]+) sec/) {
	  $r->{VALID} = 1;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } else {
	  print Dumper($out);
	  croak "cannot understand results";
     }

     #     print "\n\n".Dumper($out,$r);

     writeFile("abc.out",Dumper($out,$r));

     return $r;
}







#
# run a single instance of bmc with ABC, returns the results
#
sub  abc_pdr {
     my ($args) = @_;

     my $input_file    = $args->{"INPUT_FILE"};
     my $f             = $args->{"FRAMES"};

     print "\nstarting pdr : ".Dumper($args);
     
     writeFile("abc.script","r $input_file; \npdr -F $f -C 100000000 -v;\n");
     my $out = `abc -F abc.script`;

     my $r = {
	      "MAX_FRAMES" => $f,
	      "FRAMES" => -1,
	      "VALID" => -1,
	      "RUN_TIME" => -1,
	     };

     if ($out =~ /Reached limit on the number of timeframes \((\d+)\).[\w\W]+Property UNDECIDED.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID} = 0;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /Property DISPROVED in frame (\d+) [\w\W]+.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID} = 0;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /Invariant F\[(\d+)\] :[\w\W]+Property proved.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID} = 1;
	  $r->{FRAMES} = $1;
	  $r->{RUN_TIME} = $2;
     } else {
	  print Dumper($out);
	  writeFile("abc.out",Dumper($out,$r));
	  croak "cannot understand pdr results";
     }

     #     print "\n\n".Dumper($out,$r);

     writeFile("abc.out",Dumper($out,$r));

     return $r;
}







sub readFileArray  {
     my ( $f ) = @_;
     open F, "< $f" or die "Can't open $f : $!";
     my @f = <F>;		close F;
     return @f;
}


sub writeFile {
     my ( $f, @data ) = @_;
     @data = () unless @data;
     open F, "> $f" or die "Can't open $f : $!";
     print F @data;
     close F;
}

sub appendFile {
     my ( $f, @data ) = @_;
     @data = () unless @data;
     open F, ">> $f" or die "Can't open $f : $!";
     print F @data;
     close F;
}


sub readFileScalar  {
     my ( $f ) = @_;
     open F, "< $f" or die "Can't open $f : $!";
     my @f = <F>;		close F;
     return "@f";
}

sub makeUnique {
     my @a = @_; 	
     my %saw;
     @a = grep(!$saw{$_}++, @a);   return @a;
}



sub resultIsValid {
     my $R = shift;
     if (defined $R->{Off_Time}) {
	  return 1;
     } else {
	  #	   print "\n\nThis result deemed bad:\n".Dumper($R);
	  return 0;
     }
}
