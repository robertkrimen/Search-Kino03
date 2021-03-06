use Search::Kino03::KinoSearch;

1;

__END__

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    # (Compiler is an abstract base class.)
    package MyCompiler;
    use base qw( Search::Kino03::KinoSearch::Search::Compiler );

    sub make_matcher {
        my $self = shift;
        return MyMatcher->new( @_, compiler => $self );
    }
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR_CODE_SAMPLE';
    my $compiler = MyCompiler->SUPER::new(
        parent     => $my_query,
        searchable => $searcher,
        similarity => $sim,        # default: undef
        boost      => undef,       # default: see below
    );
END_CONSTRUCTOR_CODE_SAMPLE

{   "Search::Kino03::KinoSearch::Search::Compiler" => {
        bind_methods => [
            qw(
                Make_Matcher
                Get_Parent
                Get_Similarity
                Get_Weight
                Sum_Of_Squared_Weights
                Apply_Norm_Factor
                Normalize
                Highlight_Spans
                )
        ],
        make_constructors => ["do_new"],
        make_pod          => {
            methods => [
                qw(
                    make_matcher
                    get_weight
                    sum_of_squared_weights
                    apply_norm_factor
                    normalize
                    get_parent
                    get_similarity
                    highlight_spans
                    )
            ],
            synopsis    => $synopsis,
            constructor => { sample => $constructor },
        }
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.
