use Search::Kino03::KinoSearch;

1;

__END__

__XS__

MODULE = Search::Kino03::KinoSearch  PACKAGE = Search::Kino03::KinoSearch::Indexer

chy_i32_t
CREATE(...)
CODE:
    CHY_UNUSED_VAR(items);
    RETVAL = kino_Indexer_CREATE;
OUTPUT: RETVAL

chy_i32_t
TRUNCATE(...)
CODE:
    CHY_UNUSED_VAR(items);
    RETVAL = kino_Indexer_TRUNCATE;
OUTPUT: RETVAL

void
add_doc(self, ...)
    kino_Indexer *self;
PPCODE:
{
    kino_Doc *doc = NULL;
    SV *doc_sv = NULL;
    float boost = 1.0;

    if (items == 2) {
        doc_sv = ST(1);
    }
    else if (items > 2) {
        SV* boost_sv = NULL; 
        XSBind_allot_params( &(ST(0)), 1, items, "Search::Kino03::KinoSearch::Indexer::add_doc_PARAMS",
            &doc_sv, "doc", 3,
            &boost_sv, "boost", 5,
            NULL);
        if (boost_sv) {
            boost = SvNV(boost_sv);
        }
    }
    else if (items == 1) {
        KINO_THROW("Missing required argument 'doc'");
    }

    /* Either get a Doc or use the stock doc. */
    if (sv_isobject(doc_sv) && sv_derived_from(doc_sv, KINO_DOC.name->ptr)) {
        IV tmp = SvIV( SvRV(doc_sv) );
        doc = INT2PTR(kino_Doc*, tmp);
    }
    else if (XSBind_sv_defined(doc_sv) && SvROK(doc_sv)) {
        HV *maybe_fields = (HV*)SvRV(doc_sv);
        if (SvTYPE((SV*)maybe_fields) == SVt_PVHV) {
            doc = self->stock_doc;
            Kino_Doc_Set_Fields(doc, maybe_fields);
        }
    }
    if (!doc) {
        THROW("Need either a hashref or a %o", KINO_DOC.name);
    }

    Kino_Indexer_Add_Doc(self, doc, boost);
}

__AUTO_XS__

my $synopsis = <<'END_SYNOPSIS';
    my $indexer = Search::Kino03::KinoSearch::Indexer->new(
        schema => $schema,
        index  => '/path/to/index',
        create => 1,
    );
    while ( my ( $title, $content ) = each %source_docs ) {
        $indexer->add_doc({
            title   => $title,
            content => $content,
        });
    }
    $indexer->commit;
END_SYNOPSIS

my $constructor = <<'END_NEW';
==head2 new( I<[labeled params]> )

    my $indexer = Search::Kino03::KinoSearch::Indexer->new(
        schema       => $schema,           # required
        index        => '/path/to/index',  # required
        create       => 1,                 # default: 0
        truncate     => 1,                 # default: 0
        lock_factory => $factory           # default: created internally 
    );

==over

==item *

B<schema> - A Schema.

==item *

B<index> - Either a filepath to an index or a Folder.

==item *

B<create> - If true and the index directory does not exist, attempt to create
it.

==item *

B<truncate> - If true, proceed with the intention of discarding all previous
indexing data.  The old data will remain intact and visible until commit() is
called.

==item *

B<lock_factory> - A LockFactory.

==back
END_NEW

# Override is necessary because there's no standard way to explain
# hash/hashref across multiple host languages.
my $add_doc_pod = <<'END_ADD_DOC_POD';
==head2 add_doc(...)

    $indexer->add_doc($doc);
    $indexer->add_doc( { field_name => $field_value } );
    $indexer->add_doc(
        doc   => { field_name => $field_value },
        boost => 2.5,         #: default 1.0
    );

Add a document to the index.  Accepts either a single argument or labeled
params.

==over

==item *

B<doc> - Either a Search::Kino03::KinoSearch::Doc object, or a hashref (which will be
attached to a Search::Kino03::KinoSearch::Doc object internally).

==item *

B<boost> - A floating point weight which affects how this document scores.  

==back

END_ADD_DOC_POD

{   "Search::Kino03::KinoSearch::Indexer" => {
        bind_methods => [
            qw(
                Delete_By_Term
                Delete_By_Query
                Add_Index
                Commit
                Prepare_Commit
                Optimize
                )
        ],
        make_constructors => ["_new|init"],
        make_getters      => [qw( seg_writer )],
        make_pod          => {
            methods => [
                { name => 'add_doc', pod => $add_doc_pod },
                'add_index',
                'optimize',
                'commit',
                'prepare_commit',
                'delete_by_term',
                'delete_by_query',
            ],
            synopsis     => $synopsis,
            constructors => [$constructor],
        },
    }
}

__COPYRIGHT__

Copyright 2005-2009 Marvin Humphrey

This program is free software; you can redistribute it and/or modify
under the same terms as Perl itself.

