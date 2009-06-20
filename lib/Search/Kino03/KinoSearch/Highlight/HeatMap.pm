use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $constructor = <<'END_CONSTRUCTOR';
    my $heat_map = Search::Kino03::KinoSearch::Highlight::HeatMap->new(
        spans  => \@highlight_spans,
        window => 100,
    );
END_CONSTRUCTOR

{   "Search::Kino03::KinoSearch::Highlight::HeatMap" => {
        bind_methods => [
            qw(
                Calc_Proximity_Boost
                Generate_Proximity_Boosts
                Flatten_Spans
                )
        ],
        make_getters      => [qw( spans window )],
        make_constructors => ["new"],
        make_pod          => {
            synopsis    => "    # TODO.\n",
            constructor => { sample => $constructor },
        },
    }
}
