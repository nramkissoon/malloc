
#include <unistd.h>;

typedef long Align; /* for alignment to long boundary */

/*
    block header
    A free block contains a pointer to the next block in the free chain, a record of the size of the block, and the free space itself.
    The control information at the beginning is called the "header". The header is aligned to long.
*/
union header {
    struct {
        union header *ptr;
        unsigned size;
    } s;
    Align x;
};

typedef union header Header;

static Header base; // empty list to get started
static Header *freep = NULL; // start of free list

void *malloc(unsigned nbytes)
{
    Header *p, *prevp;
    Header *morecore(unsigned);
    unsigned nunits;

    nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
    if ((prevp = freep) == NULL) { // no free list yet
        base.s.ptr = freep = prevp = &base;
        base.s.size = 0;
    }
    for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) { // search free list
        if (p->s.size >= nunits) { // big enough
            if (p->s.size == nunits) // exact
                prevp->s.ptr = p->s.ptr;
            else { //allocate tail end
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }
            freep = prevp;
            return (void *)(p+1);
        }
        if (p == freep) // wrapped around free list
            if ((p = morecore(nunits)) == NULL)
                return NULL; // none left
    }

}

#define NALLOC 1024 // minimum #units to request

/* morecore obtains storage from the os */
static Header *morecore(unsigned nu)
{
    char *cp;
    void *sbrk(int);

    Header *up;

    if (nu < NALLOC)
        nu = NALLOC;

    cp = sbrk(nu * sizeof(Header));
    if (cp == (char *) -1) // no space, sbrk returned -1
        return NULL;
    up = (Header *) cp;
    up->s.size = nu;
    free((void *)(up + 1));
    return freep;
}

/* free: put block ap in free list */
void free(void *ap)
{
    Header *bp, *p;

    bp = (Header *)ap - 1; // point to the block header
    for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
        if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
            break; // freed block at start or end
    
    if (bp + bp->s.size == p->s.ptr) { // join to upper nbr
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    } else 
        bp->s.ptr = p->s.ptr;
    if (p + p->s.size == bp) { // join to lower nbr
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else 
        p->s.ptr = bp;
    freep = p;
}