#!/usr/bin/perl -w

use strict;
use POSIX qw(ceil);
use Data::Dumper;
use XML::Parser;
use Date::Parse;
use Date::Format;
use Time::HiRes qw(tv_interval gettimeofday);

our %I = ( Active => 0 );
our %T = ( Manual => 0 );
our %S = ( Absent => 0 );
our $F = 0;
our $D;

my $runs;
my $run;
my $lap;
my $track;
my $lpoint;
my $point;

my $file    = $ARGV[0];
my $size    = (stat($file))[7];
my $start   = [gettimeofday];
my $parser  = new XML::Parser(Style => 'Subs');
$parser->parsefile($file);
my $elapsed = tv_interval($start,[gettimeofday]);
my $mbps    = sprintf("%.2f",$size/$elapsed/1024/1024);

print "Parsed $size bytes in $elapsed seconds ($mbps MB/s)\n";

# Assign the lap and track indices among the runs, and pack the runs.

if ( $runs ) {
  my $lap_count   = 0;
  my $track_count = $#$runs;

  foreach my $x ( @$runs ) {
    $lap_count += scalar(@{$x->{laps}});
    $x->{year}  = time2str("%Y",$x->{start});
    $x->{month} = time2str("%m",$x->{start});
    $x->{date}  = time2str("%Y%m%dT%H%M%S",$x->{start});
  }
  foreach my $x ( @$runs ) {
    my $lcnt = scalar(@{$x->{laps}});
    $x->{first_lap_index} = $lap_count - $lcnt;
    $x->{last_lap_index}  = $lap_count - 1;
    $x->{track_index}     = $track_count;

    my $z = $x->{first_lap_index};
    my $e = 0;
    foreach my $y ( @{$x->{laps}} ) {
      $y->{index} = $z;
      $e = ceil($y->{start} + $y->{duration});
      $z++;
    }

    push (@{$x->{points}}, { time => $e });

    $lap_count -= $lcnt;
    $track_count--;

    my $bytes = pack_run($x);

    printf("%s: track %3d, laps %4d through %4d, %6d bytes\n",
	   $x->{date},$x->{track_index},
	   $x->{first_lap_index},$x->{last_lap_index},$bytes);
  }
}

# Now we can loop through the runs and translate them to .gmn files.


# ----------------------------------------------------------------------------
# Packing subroutines
# ----------------------------------------------------------------------------

# Pack a run into a binary string.

sub pack_run {
  my ($run) = @_;

  my $s;

  # List ID 2 is the laps.

  my $nl = scalar(@{$run->{laps}});
  my $l2 = '';
  foreach my $l ( @{$run->{laps}} ) {
    my $dl = pack_d1011($l);
    my $ls = pack('LLL',2,1011,length($dl)) . $dl;
    $l2 .= $ls;
  }
  my $ll2 = pack('LLLL',1,length($l2)+8,2,$nl) . $l2;

  # List ID 3 is the tracks (header and points).

  my $nt = scalar(@{$run->{points}});
  my $dh = pack_d311($run->{track_index});
  my $l3 = pack('LLL',3,311,length($dh)) . $dh;
  foreach my $p ( @{$run->{points}} ) {
    my $dl = pack_d304($p);
    my $ls = pack('LLL',3,304,length($dl)) . $dl;
    $l3 .= $ls;
  }
  my $ll3 = pack('LLLL',1,length($l3)+8,3,$nt+1) . $l3;

  # List ID 1 is the three-element list containing the d1009, the laps,
  # and the tracks.

  my $lr = pack_d1009($run);
  my $llr = pack('LL',1009,length($lr)) . $lr;

  my $l1 = '';
  $l1 .= pack('L',1) . $llr;
  $l1 .= pack('L',1) . $ll2;
  $l1 .= pack('L',1) . $ll3;

  my $l = pack('LLLL',1,length($l1)+8,1,3) . $l1;
  my $n = length($l);
  my $hdr = pack('Z12LL','<@gArMiN@>',100,$n);

  my $tl = length($hdr) + $n;

  if ( open(RUN,">$run->{date}.gmn") ) {
    print RUN $hdr;
    print RUN $l;
    close(RUN);
  }

  return $tl;
}

# #define DEGREES      180.0
# #define SEMICIRCLES  0x80000000
#
# #define SEMI2DEG(a)  (a) * DEGREES / SEMICIRCLES
# #define DEG2SEMI(a)  (a) * SEMICIRCLES / DEGREES
#
# typedef struct position_type {
#   sint32                lat;     /* latitude in semicircles  */
#   sint32                lon;     /* longitude in semicircles */
# } position_type;
#
# typedef struct D304 {
#   position_type     posn;
#   uint32            time;
#   float32           alt;
#   float32           distance;
#   uint8             heart_rate;
#   uint8             cadence;
#   bool              sensor;
# } D304;

sub deg2semi { return int(($_[0]/180.0) * 0x80000000); }
sub time2gtm { return $_[0] - 631065600; }

sub pack_d304 {
  my ($point) = @_;

  my $s;

  if ( defined($point->{lat}) ) {
    $s = pack('l2Lf2C3',
	      deg2semi($point->{lat}),deg2semi($point->{lon}),
	      time2gtm($point->{time}),$point->{alt},$point->{distance},
	      $point->{hr} || 0,0xff,0);
  } else {
    $s = pack('l2Lf2C3',0x7fffffff,0x7fffffff,
	      time2gtm($point->{time}),0,0,
	      0,0xff,0);
  }

  return $s;
}


# typedef struct D311 {
#   uint16        index;   /* unique among all tracks received from device */
# } D311;

sub pack_d311 {
  my ($index) = @_;

  my $s = pack('S',$index);

  return $s;
}


# typedef struct D1008 {
#   uint32                       num_valid_steps;
#   struct {
#     char                       custom_name[16];
#     float32                    target_custom_zone_low;
#     float32                    target_custom_zone_high;
#     uint16                     duration_value;
#     uint8                      intensity;
#     uint8                      duration_type;
#     uint8                      target_type;
#     uint8                      target_value;
#     uint16                     unused;
#   }                            steps[20];
#   char                         name[16];
#   uint8                        sport_type;
# } D1008;

sub pack_d1008 {
  my $s = pack('L',0);
  foreach (1..20) {
    $s .= pack('LLLSC2L2SC4S',
	       0xffffffff,0xffffffff,0xffffffff,0xffff,0xff,0,
	       0xffffffff,0xffffffff,0xffff,0xff,0xff,0xff,0xff,0);
  }
  $s .= pack('Z16C','',0);

  return $s;
}


# typedef struct D1009 {
#   uint16                       track_index;
#   uint16                       first_lap_index;
#   uint16                       last_lap_index;
#   uint8                        sport_type;
#   uint8                        program_type;
#   uint8                        multisport;
#   uint8                        unused1;
#   uint16                       unused2;
#   struct {
#     uint32                     time;
#     float32                    distance;
#   }                            quick_workout;
#   D1008                        workout;
# } D1009;

sub pack_d1009 {
  my ($run) = @_;

  my $s = pack('S3C4SL2',
	       $run->{track_index},
	       $run->{first_lap_index},
	       $run->{last_lap_index},
	       0,0,0,0,0,0xffffffff,0xffffffff) . pack_d1008();

  return $s;
}


# typedef struct D1011 {
#   uint16                       index;
#   uint16                       unused;
#   time_type                    start_time;
#   uint32                       total_time;
#   float32                      total_dist;
#   float32                      max_speed;
#   position_type                begin;
#   position_type                end;
#   uint16                       calories;
#   uint8                        avg_heart_rate;
#   uint8                        max_heart_rate;
#   uint8                        intensity;
#   uint8                        avg_cadence;
#   uint8                        trigger_method;
# } D1011;

sub pack_d1011 {
  my ($lap) = @_;

  my $s = pack('S2L2f2l4SC5',
	       $lap->{index},
	       0,
	       time2gtm($lap->{start}),
	       int($lap->{duration} * 100),
	       $lap->{distance},
	       $lap->{max_speed},
	       deg2semi($lap->{start_lat}),
	       deg2semi($lap->{start_lon}),
	       deg2semi($lap->{end_lat}),
	       deg2semi($lap->{end_lon}),
	       $lap->{calories},
	       $lap->{avg_hr},
	       $lap->{max_hr},
	       $lap->{intensity},
	       0xff,
	       $lap->{trigger});

  return $s;
}


# ----------------------------------------------------------------------------
# XML tag handlers.
# ----------------------------------------------------------------------------

sub Run { $run = {}; }
sub Run_ {
  $run->{start} = $run->{laps}[0]{start};
  push ( @$runs, $run );
}

sub Lap {
  my ($p, $el, %atts) = @_;
  $lap = { start => str2time($atts{StartTime}) };
}
sub Lap_ { push ( @{$run->{laps}}, $lap ); }

sub TotalTimeSeconds     { save_on(shift);  }
sub TotalTimeSeconds_    { $lap->{duration} = $D; save_off(shift); }

sub DistanceMeters       { save_on(shift);  }
sub DistanceMeters_ {
  if ( defined($point) ) {
    $point->{distance} = $D;
  } else {
    $lap->{distance} = $D;
  }
  save_off(shift);
}

sub MaximumSpeed         { save_on(shift);  }
sub MaximumSpeed_        { $lap->{max_speed} = $D; save_off(shift); }

sub Calories             { save_on(shift);  }
sub Calories_            { $lap->{calories} = $D; save_off(shift); }

sub AverageHeartRateBpm  { save_on(shift);  }
sub AverageHeartRateBpm_ { $lap->{avg_hr} = $D; save_off(shift); }

sub MaximumHeartRateBpm  { save_on(shift);  }
sub MaximumHeartRateBpm_ { $lap->{max_hr} = $D; save_off(shift); }

sub Intensity            { save_on(shift);  }
sub Intensity_           { $lap->{intensity} = $I{$D} || 0; save_off(shift); }

sub Cadence              { save_on(shift);  }
sub Cadence_             { $lap->{cadence} = $D; save_off(shift); }

sub TriggerMethod        { save_on(shift);  }
sub TriggerMethod_       { $lap->{trigger} = $T{$D} || 0; save_off(shift); }

sub Track  { $F = 1; }
sub Track_ { 
  $lap->{end_lat} = $lpoint->{lat};
  $lap->{end_lon} = $lpoint->{lon};
}

sub Trackpoint           { $point = {}; }
sub Trackpoint_ {
  if ( not defined($lpoint) or
       $lpoint->{lat} ne $point->{lat} or
       $lpoint->{lon} ne $point->{lon} or
       $lpoint->{alt} ne $point->{alt} or
       $lpoint->{distance} ne $point->{distance} or
       $lpoint->{time} ne $point->{time} or
       $lpoint->{hr} ne $point->{hr} ) {
    push (@{$run->{points}}, $point);
  }
  if ( $F == 1 ) {
    $lap->{start_lat} = $point->{lat};
    $lap->{start_lon} = $point->{lon};
    $F = 0;
  }
  $lpoint = $point;
  undef($point);
}

sub Time                 { save_on(shift); }
sub Time_                { $point->{time} = str2time($D); save_off(shift); }

sub LatitudeDegrees      { save_on(shift); }
sub LatitudeDegrees_     { $point->{lat} = $D; save_off(shift); }

sub LongitudeDegrees     { save_on(shift); }
sub LongitudeDegrees_    { $point->{lon} = $D; save_off(shift); }

sub AltitudeMeters       { save_on(shift); }
sub AltitudeMeters_      { $point->{alt} = $D; save_off(shift); }

sub HeartRateBpm         { save_on(shift); }
sub HeartRateBpm_        { $point->{hr} = $D; save_off(shift); }

sub SensorState          { save_on(shift); }
sub SensorState_         { $point->{sensor} = $S{$D} || 0; save_off(shift); }

sub Save                 { $D .= $_[1]; }

sub save_on              { (shift)->setHandlers('Char' => \&Save); $D = ''; }
sub save_off             { (shift)->setHandlers('Char' => undef); }
