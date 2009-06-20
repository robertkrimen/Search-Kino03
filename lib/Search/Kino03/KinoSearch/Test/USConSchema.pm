use strict;
use warnings;

package Search::Kino03::KinoSearch::Test::USConSchema;
use base 'Search::Kino03::KinoSearch::Schema';
use Search::Kino03::KinoSearch::Analysis::PolyAnalyzer;
use Search::Kino03::KinoSearch::FieldType::FullTextType;
use Search::Kino03::KinoSearch::FieldType::StringType;

sub new {
    my $self = shift->SUPER::new(@_);
    my $analyzer
        = Search::Kino03::KinoSearch::Analysis::PolyAnalyzer->new( language => 'en' );
    my $title_type
        = Search::Kino03::KinoSearch::FieldType::FullTextType->new( analyzer => $analyzer, );
    my $content_type = Search::Kino03::KinoSearch::FieldType::FullTextType->new(
        analyzer      => $analyzer,
        highlightable => 1,
    );
    my $url_type = Search::Kino03::KinoSearch::FieldType::StringType->new( indexed => 0, );
    my $cat_type = Search::Kino03::KinoSearch::FieldType::StringType->new;
    $self->spec_field( name => 'title',    type => $title_type );
    $self->spec_field( name => 'content',  type => $content_type );
    $self->spec_field( name => 'url',      type => $url_type );
    $self->spec_field( name => 'category', type => $cat_type );
    return $self;
}

1;
