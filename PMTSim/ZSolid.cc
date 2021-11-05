#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "G4UnionSolid.hh"
#include "G4IntersectionSolid.hh"
#include "G4SubtractionSolid.hh"

#include "G4Ellipsoid.hh"
#include "G4Tubs.hh"
#include "G4Polycone.hh"
#include "G4RotationMatrix.hh"

#include "ZCanvas.hh"
#include "ZSolid.hh"
#include "NP.hh"


G4VSolid* ZSolid::CreateZCutTree( const G4VSolid* original, double zcut ) // static
{
    std::cout << "ZSolid::CreateZCutTree" << std::endl ; 
    ZSolid* zs = new ZSolid(original); 
    zs->cutTree( zcut );  
    return zs->root ; 
}

ZSolid::ZSolid(const G4VSolid* original_ ) 
    :
    original(original_),
    root(DeepClone(original_)),
    candidate_root(nullptr),
    parent_map(new std::map<const G4VSolid*, const G4VSolid*>),

    in_map(    new std::map<const G4VSolid*, int>),
    rin_map(   new std::map<const G4VSolid*, int>),
    pre_map(   new std::map<const G4VSolid*, int>),
    rpre_map(  new std::map<const G4VSolid*, int>),
    post_map(  new std::map<const G4VSolid*, int>),
    rpost_map( new std::map<const G4VSolid*, int>),

    zcls_map(  new std::map<const G4VSolid*, int>),
    depth_map( new std::map<const G4VSolid*, int>),

    width(0),
    height(0) 
{
    init(); 
}

void ZSolid::init()
{
    initTree(); 
    dumpTree("ZSolid::init.dumpTree"); 
    dumpUp("ZSolid::init.dumpUp"); 
}

void ZSolid::initTree() 
{
    std::cout << "ZSolid::initTree" << std::endl ; 
    fillParentMap(); 

    depth_r(root, 0 ); 

    inorder.clear();
    inorder_r(root, 0 ); 

    rinorder.clear();
    rinorder_r(root, 0 ); 

    preorder.clear();
    preorder_r(root, 0 ); 

    rpreorder.clear();
    rpreorder_r(root, 0 ); 

    postorder.clear(); 
    postorder_r(root, 0 ); 

    rpostorder.clear(); 
    rpostorder_r(root, 0 ); 

    width = inorder.size(); 
    height = maxdepth() ; 
    canvas = new ZCanvas(width, height); 
}


/**
ZSolid::fillParentMap
-----------------------

Note that the parent_map uses the G4DisplacedSolid (not the G4VSolid that it points to) 
in order to have treewise access to the transform up the lineage. 

**/

void ZSolid::fillParentMap()
{
    (*parent_map)[root] = nullptr ;  // root has no parent by definition
    fillParentMap_r( root ); 
}

void ZSolid::fillParentMap_r( const G4VSolid* node)
{
    if(Boolean(node))
    {
        const G4VSolid* l = Left(node) ; 
        const G4VSolid* r = Right(node) ; 

        fillParentMap_r( l );  
        fillParentMap_r( r );  

        // postorder visit 
        (*parent_map)[l] = node ; 
        (*parent_map)[r] = node ; 
    }
}

void ZSolid::depth_r(const G4VSolid* node_, int depth)
{
    if( node_ == nullptr ) return ; 
    depth_r(Left(node_), depth+1) ; 
    depth_r(Right(node_), depth+1) ; 
    (*depth_map)[node_] = depth ;   
}

void ZSolid::inorder_r(const G4VSolid* node_, int depth)
{
    if( node_ == nullptr ) return ; 

    inorder_r(Left(node_), depth+1) ; 

    (*in_map)[node_] = inorder.size();   
    inorder.push_back(node_) ; 

    inorder_r(Right(node_), depth+1) ; 
} 

void ZSolid::rinorder_r(const G4VSolid* node_, int depth)
{
    if( node_ == nullptr ) return ; 

    rinorder_r(Right(node_), depth+1) ; 

    (*rin_map)[node_] = rinorder.size();   
    rinorder.push_back(node_) ; 

    rinorder_r(Left(node_), depth+1) ; 
} 

void ZSolid::preorder_r(const G4VSolid* node_, int depth)
{
    if( node_ == nullptr ) return ; 

    (*pre_map)[node_] = preorder.size();   
    preorder.push_back(node_) ; 

    preorder_r(Left(node_), depth+1) ; 
    preorder_r(Right(node_), depth+1) ; 
} 

void ZSolid::rpreorder_r(const G4VSolid* node_, int depth)
{
    if( node_ == nullptr ) return ; 

    (*rpre_map)[node_] = rpreorder.size();   
    rpreorder.push_back(node_) ; 

    rpreorder_r(Right(node_), depth+1) ; 
    rpreorder_r(Left(node_), depth+1) ; 
} 

void ZSolid::postorder_r(const G4VSolid* node_, int depth)
{
    if( node_ == nullptr ) return ; 
    postorder_r(Left(node_), depth+1) ; 
    postorder_r(Right(node_), depth+1) ; 

    (*post_map)[node_] = postorder.size();   
    postorder.push_back(node_) ; 
}

void ZSolid::rpostorder_r(const G4VSolid* node_, int depth)
{
    if( node_ == nullptr ) return ; 
    rpostorder_r(Right(node_), depth+1) ; 
    rpostorder_r(Left(node_), depth+1) ; 

    (*rpost_map)[node_] = rpostorder.size();   
    rpostorder.push_back(node_) ; 
}

int ZSolid::in(    const G4VSolid* node_) const { return (*in_map)[node_] ; }
int ZSolid::rin(   const G4VSolid* node_) const { return (*rin_map)[node_] ; }
int ZSolid::pre(   const G4VSolid* node_) const { return (*pre_map)[node_] ; }
int ZSolid::rpre(  const G4VSolid* node_) const { return (*rpre_map)[node_] ; }
int ZSolid::post(  const G4VSolid* node_) const { return (*post_map)[node_] ; }
int ZSolid::rpost( const G4VSolid* node_) const { return (*rpost_map)[node_] ; }

int ZSolid::depth(const G4VSolid* node_) const { return (*depth_map)[node_] ; }

int ZSolid::zcls( const G4VSolid* node_, bool move) const 
{ 
    const G4VSolid* node = move ? Moved(nullptr, nullptr, node_ ) : node_ ; 
    return zcls_map->count(node) == 1 ? (*zcls_map)[node] : UNDEFINED ; 
}

void ZSolid::set_zcls( const G4VSolid* node_, bool move, int zc )
{
    const G4VSolid* node = move ? Moved(nullptr, nullptr, node_ ) : node_ ; 
    (*zcls_map)[node] = zc  ;
} 

void ZSolid::draw(const char* msg) 
{
    canvas->clear();

    int mode = RPRE ; 
    std::cout << msg << std::endl ; 
    std::cout << "OrderName " << OrderName(mode) << std::endl ; 

    draw_r(root, mode);

    canvas->print(); 
}

void ZSolid::draw_r( const G4VSolid* n, int mode )
{
    if( n == nullptr ) return ;
    draw_r( Left(n),  mode );
    draw_r( Right(n), mode );

    int x = in(n) ;            // inorder index, aka "side", increasing from left to right 
    int y = depth(n) ;         // increasing downwards
    int idx = index(n, mode);  // index for presentation 

    const char* tag = EntityTag(n, true) ; 
    int zcl = zcls(n, true); 
    const char* zcn = ClassifyMaskName(zcl) ; 

    canvas->draw( x, y, tag, 0); 
    canvas->draw( x, y, zcn, 1); 
    canvas->draw( x, y, idx, 2); 
}

/**
ZSolid::index
---------------

IN inorder
   left-to-right index (aka side) 

RIN reverse inorder
   right-to-left index 

PRE preorder
   with left unbalanced tree this does a anti-clockwise rotation starting from root
   [by observation is opposite to RPOST]

RPRE reverse preorder
   with left unbalanced does curios zig-zag that looks to be exact opposite of 
   the familiar postorder traversal, kinda undo of the postorder.
   [by observation is opposite to POST]

   **COULD BE USEFUL FOR TREE EDITING FROM RIGHT** 

POST postorder
   old familar traversal starting from bottom left 
   [by observation is opposite to RPRE]

RPOST reverse postorder
   with left unbalanced does a clockwise cycle starting
   from rightmost visiting all prim before getting 
   to any operators
   [by observation is opposite to PRE]

**/

int ZSolid::index( const G4VSolid* n, int mode ) const  
{
    int idx = -1 ; 
    switch(mode)
    {
        case IN:    idx = in(n)    ; break ; 
        case RIN:   idx = rin(n)   ; break ; 
        case PRE:   idx = pre(n)   ; break ; 
        case RPRE:  idx = rpre(n)  ; break ; 
        case POST:  idx = post(n)  ; break ; 
        case RPOST: idx = rpost(n) ; break ;  
    }
    return idx ; 
}

const char* ZSolid::IN_ = "IN" ; 
const char* ZSolid::RIN_ = "RIN" ; 
const char* ZSolid::PRE_ = "PRE" ; 
const char* ZSolid::RPRE_ = "RPRE" ; 
const char* ZSolid::POST_ = "POST" ; 
const char* ZSolid::RPOST_ = "RPOST" ; 

const char* ZSolid::OrderName(int mode) // static
{
    const char* s = nullptr ; 
    switch(mode)
    {
        case IN:    s = IN_    ; break ; 
        case RIN:   s = RIN_   ; break ; 
        case PRE:   s = PRE_   ; break ; 
        case RPRE:  s = RPRE_  ; break ; 
        case POST:  s = POST_  ; break ; 
        case RPOST: s = RPOST_ ; break ;  
    }
    return s ; 
}

/**
ZSolid::EntityTypeName
-----------------------

Unexpectedly G4 returns EntityType by value rather than by reference
so have to strdup to avoid corruption when the G4String goes out of scope. 

**/

const char* ZSolid::EntityTypeName(const G4VSolid* solid)   // static
{
    G4GeometryType type = solid->GetEntityType();  // G4GeometryType typedef for G4String
    return strdup(type.c_str()); 
}

const char* ZSolid::EntityTag(const G4VSolid* node_, bool move)   // static
{
    const G4VSolid*  node = move ? Moved(nullptr, nullptr, node_ ) : node_ ; 
    G4GeometryType type = node->GetEntityType();  // G4GeometryType typedef for G4String
    char* tag = strdup(type.c_str() + 2);  // +2 skip "G4"
    assert( strlen(tag) > 3 ); 
    tag[3] = '\0' ;
    return tag ;  
}

int ZSolid::EntityType(const G4VSolid* solid)   // static 
{
    const char* name = EntityTypeName(solid); 
    int type = _G4Other ; 
    if( strcmp(name, "G4Ellipsoid") == 0 )         type = _G4Ellipsoid ; 
    if( strcmp(name, "G4Tubs") == 0 )              type = _G4Tubs ; 
    if( strcmp(name, "G4Polycone") == 0 )          type = _G4Polycone ; 
    if( strcmp(name, "G4UnionSolid") == 0 )        type = _G4UnionSolid ; 
    if( strcmp(name, "G4SubtractionSolid") == 0 )  type = _G4SubtractionSolid ; 
    if( strcmp(name, "G4IntersectionSolid") == 0 ) type = _G4IntersectionSolid ; 
    if( strcmp(name, "G4DisplacedSolid") == 0 )    type = _G4DisplacedSolid ; 
    return type ; 
}

bool ZSolid::Boolean(const G4VSolid* solid) // static
{
    return dynamic_cast<const G4BooleanSolid*>(solid) != nullptr ; 
}
bool ZSolid::Displaced(const G4VSolid* solid) // static
{
    return dynamic_cast<const G4DisplacedSolid*>(solid) != nullptr ; 
}
const G4VSolid* ZSolid::Left(const G4VSolid* solid ) // static
{
    return Boolean(solid) ? solid->GetConstituentSolid(0) : nullptr ; 
}
const G4VSolid* ZSolid::Right(const G4VSolid* solid ) // static
{
    return Boolean(solid) ? solid->GetConstituentSolid(1) : nullptr ; 
}
G4VSolid* ZSolid::Left_(G4VSolid* solid ) // static
{
    return Boolean(solid) ? solid->GetConstituentSolid(0) : nullptr ; 
}
G4VSolid* ZSolid::Right_(G4VSolid* solid ) // static
{
    return Boolean(solid) ? solid->GetConstituentSolid(1) : nullptr ; 
}

/**
ZSolid::Moved
---------------

When node isa G4DisplacedSolid sets the rotation and translation and returns the constituentMovedSolid
otherwise returns the input node.

**/
const G4VSolid* ZSolid::Moved( G4RotationMatrix* rot, G4ThreeVector* tla, const G4VSolid* node )  // static
{
    const G4DisplacedSolid* disp = dynamic_cast<const G4DisplacedSolid*>(node) ; 
    if(disp)
    {
        if(rot) *rot = disp->GetFrameRotation();
        if(tla) *tla = disp->GetObjectTranslation();  
    }
    return disp ? disp->GetConstituentMovedSolid() : node  ;
}

G4VSolid* ZSolid::Moved_( G4RotationMatrix* rot, G4ThreeVector* tla, G4VSolid* node )  // static
{
    G4DisplacedSolid* disp = dynamic_cast<G4DisplacedSolid*>(node) ; 
    if(disp)
    {
        if(rot) *rot = disp->GetFrameRotation();
        if(tla) *tla = disp->GetObjectTranslation();  
    }
    return disp ? disp->GetConstituentMovedSolid() : node  ;
}

int ZSolid::maxdepth() const  
{
    return Maxdepth_r( root, 0 );  
}

int ZSolid::Maxdepth_r( const G4VSolid* node_, int depth)  // static 
{
    return Boolean(node_) ? std::max( Maxdepth_r(ZSolid::Left(node_), depth+1), Maxdepth_r(ZSolid::Right(node_), depth+1)) : depth ; 
}

/**
ZSolid::dumpTree
------------------
 
Postorder traversal of CSG tree
**/
void ZSolid::dumpTree(const char* msg ) const 
{
    std::cout << msg << " maxdepth(aka height) " << height << std::endl ; 
    dumpTree_r(root, 0 ); 
}

void ZSolid::dumpTree_r( const G4VSolid* node_, int depth  ) const
{
    if(Boolean(node_))
    {
        dumpTree_r(ZSolid::Left(node_) , depth+1) ; 
        dumpTree_r(ZSolid::Right(node_), depth+1) ; 
    }

    // postorder visit 

    G4RotationMatrix node_rot ;   // node (not tree) transforms
    G4ThreeVector    node_tla(0., 0., 0. ); 
    const G4VSolid*  node = Moved(&node_rot, &node_tla, node_ ); 
    assert( node ); 

    double zdelta_always_zero = getZ(node); 
    assert( zdelta_always_zero == 0. ); 
    // Hmm thats tricky, using node arg always gives zero.
    // Must use node_ which is the one that might be G4DisplacedSolid. 

    double zdelta = getZ(node_) ;  

    std::cout 
        << " type " << std::setw(20) << EntityTypeName(node) 
        << " name " << std::setw(20) << node->GetName() 
        << " depth " << depth 
        << " zdelta "
        << std::fixed << std::setw(7) << std::setprecision(2) << zdelta
        << " node_tla (" 
        << std::fixed << std::setw(7) << std::setprecision(2) << node_tla.x() << " " 
        << std::fixed << std::setw(7) << std::setprecision(2) << node_tla.y() << " "
        << std::fixed << std::setw(7) << std::setprecision(2) << node_tla.z() << ")"
        ;

    if(ZSolid::CanZ(node))
    {
        double z0, z1 ; 
        ZRange(z0, z1, node);  
        std::cout 
            << " z1 " << std::fixed << std::setw(7) << std::setprecision(2) << z1
            << " z0 " << std::fixed << std::setw(7) << std::setprecision(2) << z0 
            << " az1 " << std::fixed << std::setw(7) << std::setprecision(2) << ( z1 + zdelta )
            << " az0 " << std::fixed << std::setw(7) << std::setprecision(2) << ( z0 + zdelta )
            ;
    }
    std::cout 
        << std::endl
        ; 
}

/**
ZSolid::classifyTree
----------------------

postorder traveral classifies every node doing bitwise-OR
combination of the child classifications.

**/

int ZSolid::classifyTree(double zcut)  
{
    std::cout << "ZSolid::classifyTree against zcut " << zcut  << std::endl ; 
    int zc = classifyTree_r(root, 0, zcut); 
    return zc ; 
}

int ZSolid::classifyTree_r( const G4VSolid* node_, int depth, double zcut )
{
    int zcl = 0 ; 
    int sid = in(node_);    // inorder 
    int pos = post(node_); 

    if(Boolean(node_))
    {
        int left_zcl = classifyTree_r(Left(node_) , depth+1, zcut) ; 
        int right_zcl = classifyTree_r(Right(node_), depth+1, zcut) ; 

        zcl |= left_zcl ; 
        zcl |= right_zcl ; 

        if(false) std::cout 
            << "ZSolid::classifyTree_r" 
            << " sid " << std::setw(2) << sid
            << " pos " << std::setw(2) << pos
            << " left_zcl " << ClassifyMaskName(left_zcl)
            << " right_zcl " << ClassifyMaskName(right_zcl)
            << " zcl " << ClassifyMaskName(zcl)
            << std::endl
            ;

        set_zcls( node_ , true, zcl ); 
    }
    else
    {
        double zd = getZ(node_) ;  
        const G4VSolid*  node = Moved(nullptr, nullptr, node_ ); 
        if(CanZ(node))
        {
            double z0, z1 ; 
            ZRange(z0, z1, node);  

            double az0 =  z0 + zd ; 
            double az1 =  z1 + zd ; 

            zcl = ClassifyZCut( az0, az1, zcut ); 

            if(false) std::cout 
                << "ZSolid::classifyTree_r"
                << " sid " << std::setw(2) << sid
                << " pos " << std::setw(2) << pos
                << " zd " << std::fixed << std::setw(7) << std::setprecision(2) << zd
                << " z1 " << std::fixed << std::setw(7) << std::setprecision(2) << z1
                << " z0 " << std::fixed << std::setw(7) << std::setprecision(2) << z0 
                << " az1 " << std::fixed << std::setw(7) << std::setprecision(2) << az1
                << " az0 " << std::fixed << std::setw(7) << std::setprecision(2) << az0 
                << " zcl " << ClassifyName(zcl)
                << std::endl
                ;

            set_zcls( node_ , true, zcl ); 
        }
    }
    return zcl ; 
}

int ZSolid::classifyMask(const G4VSolid* top) const   // NOT USED ?
{
    return classifyMask_r(top, 0); 
}
int ZSolid::classifyMask_r( const G4VSolid* node_, int depth ) const 
{
    int mask = 0 ; 
    if(Boolean(node_))
    {
        mask |= classifyMask_r( Left(node_) , depth+1 ) ; 
        mask |= classifyMask_r( Right(node_), depth+1 ) ; 
    }
    else
    {
        mask |= zcls(node_, true) ; 
    }
    return mask ; 
}

/**
ZSolid::ClassifyZCut
------------------------

Inclusion status of solid with regard to a particular zcut::

                       --- 
                        .
                        .   EXCLUDE  : zcut entirely above the solid
                        .
                        .
      +---zd+z1----+   --- 
      |            |    .   
      | . zd . . . |    .   STRADDLE : zcut within z range of solid
      |            |    .
      +---zd+z0 ---+   ---
                        .
                        .   INCLUDE  : zcut fully below the solid 
                        .
                        .
                       ---  

**/

int ZSolid::ClassifyZCut( double az0, double az1, double zcut ) // static
{
    assert( az1 > az0 ); 
    int cls = UNDEFINED ; 
    if(       zcut <= az0 )              cls = INCLUDE ; 
    else if ( zcut < az1 && zcut > az0 ) cls = STRADDLE ; 
    else if ( zcut >= az1              ) cls = EXCLUDE ; 
    return cls ; 
}

const char* ZSolid::UNDEFINED_ = "UNDEFINED" ; 
const char* ZSolid::INCLUDE_   = "INCLUDE" ; 
const char* ZSolid::STRADDLE_  = "STRADDLE" ; 
const char* ZSolid::EXCLUDE_   = "EXCLUDE" ; 

const char* ZSolid::ClassifyName( int zcl ) // static 
{
    const char* s = nullptr ; 
    switch( zcl )
    {
        case UNDEFINED: s = UNDEFINED_ ; break ; 
        case INCLUDE  : s = INCLUDE_ ; break ; 
        case STRADDLE : s = STRADDLE_ ; break ; 
        case EXCLUDE  : s = EXCLUDE_ ; break ; 
    }
    return s ; 
}

const char* ZSolid::ClassifyMaskName( int zcl ) // static
{
    std::stringstream ss ; 

    if( zcl == UNDEFINED ) ss << UNDEFINED_[0] ; 
    if( zcl & INCLUDE )  ss << INCLUDE_[0] ; 
    if( zcl & STRADDLE ) ss << STRADDLE_[0] ; 
    if( zcl & EXCLUDE )  ss << EXCLUDE_[0] ; 
    std::string s = ss.str(); 
    return strdup(s.c_str()); 
}

/**
ZSolid::cutTree
------------------

NTreeAnalyse height 7 count 15::

                                                     [un]    

                                              un         [cy]  

                                      un          cy        

                              un          zs                

                      un          cy                        

              un          co                                

      un          zs                                        

  zs      cy                                                


Could kludge cut unbalanced trees like the above simply by changing the root.
But how to cut more generally such as with a balanced tree.

When left and right child are EXCLUDED that exclusion 
needs to spread upwards to the parent. 

* classifyTree does this 

When the right child of a union gets EXCLUDED [cy] need to 
pullup the left child to replace the parent node.::


           un                              un
                                  
                    un                              cy
                                 -->               /
               cy      [cy]                    ..         .. 


More generally when the right subtree of a union *un* 
is all excluded need to pullup the left subtree ^un^ to replace the *un*::


                             (un)                            

              un                             *un*            
                                           ^ 
      un              un             ^un^            [un]   

  zs      cy      zs      co      cy      zs     [cy]    [cy]


How to do that exactly ? 

* get parent of *un* -> (un)
* change right child of (un) from *un* to ^un^

* BUT there is no G4BooleanSolid::SetConstituentSolid
  so will need to use placement new to put a new
  boolean object at the old memory address 

  * see sysrap/tests/PlacementNewTest.cc


When the right subtree of root is all excluded need to 
do the same in principal : pullup the left subtree to replace root *un*.
In practice this is simpler because can just change the root pointer::


                             *un*                           

             ^un^                            [un]           

      un              un             [un]            [un]   

  zs      cy      zs      co     [cy]    [zs]    [cy]    [cy]



How to detect can just shift the root pointer ? 


Hmm if exclusions were scattered all over would be easier to 
just rebuild. 
 

So the steps are:

1. classify the nodes of the tree against the zcut 
2. change STRADDLE node params and transforms according to the zcut
   and set classification to INCLUDE
3. edit the tree to remove the EXCLUDE nodes

**/
void ZSolid::cutTree(double zcut)
{
    std::cout << "ZSolid::cutTree " << zcut << std::endl ; 
    classifyTree(zcut);  
    //draw("[1] ZSolid::cutTree before cutTree_r  "); 
    cutTree_r(root, 0, zcut); 
    classifyTree(zcut);  
    draw("[2] ZSolid::cutTree after cutTree_r and re-classify  "); 

    findCandidateRoot(); 
}

void ZSolid::cutTree_r( G4VSolid* node_, int depth, double zcut )
{
    if(Boolean(node_))
    {
        cutTree_r( ZSolid::Left_(node_) , depth+1, zcut ) ; 
        cutTree_r( ZSolid::Right_(node_), depth+1, zcut ) ; 
    }
    else
    {
        // get tree frame zcut into local frame of the node 
        double zdelta = getZ(node_) ; 
        double local_zcut = zcut - zdelta ; 

        int zcl = zcls(node_, true );   

        if( zcl == STRADDLE )
        {
            ApplyZCut( node_, local_zcut ); 

            // instead of manually changing STRADDLE->INCLUDE, 
            // now just redo tree classification so should see the status 
            // change via the changed z range.
        } 
    }
}

/**
ZSolid::findCandidateRoot
---------------------------

Reverse (right before left) preorder traversal, 
which is exact opposite of postorder traversal. 
It is effectively an "undo" of the postorder. 

**/

void ZSolid::findCandidateRoot()  
{
    candidate_root = nullptr ; 
    findCandidateRoot_r( root, 0 ); 

    

}

void ZSolid::findCandidateRoot_r(const G4VSolid* n, int depth)
{
    if(n == nullptr) return ; 
    int zcl = zcls(n, true );   


    const char* zcn = ClassifyMaskName(zcl) ; 

    int post_idx = index(n, POST);  
    int rpre_idx = index(n, RPRE);  

    std::cout 
        << "ZSolid::findCandidateRoot_r"
        << " rpre_idx " << std::setw(3) << rpre_idx
        << " post_idx " << std::setw(3) << post_idx
        << " sum_idx " << std::setw(3) <<  (rpre_idx + post_idx)
        << " zcn " << std::setw(5) << zcn 
        << std::endl
        ;

    if( zcl == INCLUDE ) 
    {
        candidate_root = n ; 
        return ; 
    }

    findCandidateRoot_r( Right(n), depth+1 );
    findCandidateRoot_r( Left(n),  depth+1 );
}




void ZSolid::collectNodes( std::vector<const G4VSolid*>& nodes, const G4VSolid* top, int query_zcls  )
{
    collectNodes_r(nodes, top, 0, query_zcls);  
}

void ZSolid::collectNodes_r( std::vector<const G4VSolid*>& nodes, const G4VSolid* node_, int query_zcls, int depth  )
{
    if(Boolean(node_))
    {
        collectNodes_r( nodes, Left(node_) , query_zcls, depth+1 ) ; 
        collectNodes_r( nodes, Right(node_), query_zcls, depth+1 ) ; 
    }
    else
    {
        int zcl = zcls(node_, true) ; 
        if( zcl == query_zcls )
        {
            nodes.push_back(node_) ; // node_ or node ?
        } 
    }
} 





void ZSolid::ApplyZCut( G4VSolid* node_, double local_zcut ) // static
{
    G4VSolid* node = Moved_(nullptr, nullptr, node_ ); 

    std::cout << "ZSolid::ApplyZCut " << EntityTypeName(node) << std::endl ; 

    switch(EntityType(node))
    {
        case _G4Ellipsoid: ApplyZCut_G4Ellipsoid( node  , local_zcut);  break ; 
        case _G4Tubs:      ApplyZCut_G4Tubs(      node_ , local_zcut);  break ;  // cutting tubs requires changing the transform, hence node_
        case _G4Polycone:  ApplyZCut_G4Polycone(  node  , local_zcut);  break ; 
        default: 
            { 
                std::cout 
                    << "ZSolid::ApplyZCut FATAL : not implemented for entityType " 
                    << EntityTypeName(node) 
                    << std::endl ; 
                assert(0) ; 
            } ;
    }
}

/**
ZSolid::ApplyZCut_G4Ellipsoid
--------------------------------
     
::

    local                                             absolute 
    frame                                             frame    

    z1  +-----------------------------------------+    zd   
         \                                       /
           
            .                              .
    _________________________________________________ zcut 
                .                     .
                                 
    z0                 .      .   
                         
                                     
**/


void ZSolid::ApplyZCut_G4Ellipsoid( G4VSolid* node, double local_zcut)
{  
    G4Ellipsoid* ellipsoid =  dynamic_cast<G4Ellipsoid*>(node) ;  
    assert(ellipsoid); 

    double z0 = ellipsoid->GetZBottomCut() ; 
    double z1 = ellipsoid->GetZTopCut() ;
    
    double new_z0 = local_zcut ; 
    assert( new_z0 >= z0 && new_z0 < z1 ); 
    double new_z1 = z1 ; 

    ellipsoid->SetZCuts( new_z0, new_z1 ); 
}


/**
ZSolid::ApplyZCut_G4Polycone
------------------------------

Currently limited to only 2 Z-planes, 
to support more that 2 would need to delve 
into the r-z details which should be straightforward, 
just it is not yet implemented. 

**/

void ZSolid::ApplyZCut_G4Polycone( G4VSolid* node, double local_zcut)
{  
    G4Polycone* polycone = dynamic_cast<G4Polycone*>(node) ;  
    assert(polycone); 
    G4PolyconeHistorical* pars = polycone->GetOriginalParameters(); 

    unsigned num_z = pars->Num_z_planes ; 
    for(unsigned i=1 ; i < num_z ; i++)
    {
        double z0 = pars->Z_values[i-1] ; 
        double z1 = pars->Z_values[i] ; 
        assert( z1 > z0 );   
    }

    assert( num_z == 2 );    // simplifying assumption 
    pars->Z_values[0] = local_zcut ;   // get into polycone local frame

    polycone->SetOriginalParameters(pars); // not necessary, its a pointer 
}


/**
ZSolid::ApplyZCut_G4Tubs
----------------------------

Cutting G4Tubs::


     zd+hz  +---------+               +---------+     new_zd + new_hz
            |         |               |         |  
            |         |               |         |
            |         |               |         |
            |         |             __|_________|__   new_zd
            |         |               |         |
     zd   --|---------|--             |         |
            |         |               |         |
            |         |               |         |
         .  | . . . . | . .zcut . . . +---------+ . . new_zd - new_hz  . . . . . .
            |         | 
            |         |
    zd-hz   +---------+ 


     cut position

          zcut = new_zd - new_hz 
          new_zd = zcut + new_hz  


     original height:  2*hz                         
      
     cut height :     

          loc_zcut = zcut - zd 

          2*new_hz = 2*hz - (zcut-(zd-hz)) 

                   = 2*hz - ( zcut - zd + hz )

                   = 2*hz -  zcut + zd - hz 

                   = hz + zd - zcut 

                   = hz - (zcut - zd)             

                   = hz - loc_zcut 


                                    hz + zd - zcut
                        new_hz =  -----------------
                                         2

                                    hz - loc_zcut 
                        new_hz =  --------------------      new_hz( loc_zcut:-hz ) = hz     unchanged
                                          2                 new_hz( loc_zcut:0   ) = hz/2   halved
                                                            new_hz( loc_zcut:+hz ) =  0     made to disappear 


       +hz  +---------+               +---------+     zoff + new_hz
            |         |               |         |  
            |         |               |         |
            |         |               |         |
            |         |             __|_________|__   zoff 
            |         |               |         |
        0 --|---------|- . . . . . . . . . . . . . . . 0 
            |         |               |         |
            |         |               |         |
   loc_zcut | . . . . | . . . . .  .  +---------+ . . zoff - new_hz  . . . . . .
            |         | 
            |         |
      -hz   +---------+. . . . . . . . . . . . 


            loc_zcut = zoff - new_hz

                zoff = loc_zcut + new_hz 

                     = loc_zcut +  (hz - loc_zcut)/2

                     =  2*loc_zcut + (hz - loc_zcut)
                        ------------------------------
                                   2

               zoff  =  loc_zcut + hz                   zoff( loc_zcut:-hz ) = 0      unchanged
                        ---------------                 zoff( loc_zcut:0   ) = hz/2   
                              2                         zoff( loc_zcut:+hz ) = hz     makes sense, 
                                                                                      think about just before it disappears

**/


void ZSolid::ApplyZCut_G4Tubs( G4VSolid* node_ , double local_zcut )
{ 
    G4VSolid* node = Moved_(nullptr, nullptr, node_ ); 
    G4Tubs* tubs = dynamic_cast<G4Tubs*>(node) ;  
    assert(tubs); 

    double hz = tubs->GetZHalfLength() ; 
    double new_hz = (hz - local_zcut)/2. ;  
    tubs->SetZHalfLength(new_hz);  

    double zoffset = (local_zcut + hz)/2. ; 

    G4DisplacedSolid* disp = dynamic_cast<G4DisplacedSolid*>(node_) ; 
    assert( disp ); // transform must be associated as must change offset to cut G4Tubs

    G4ThreeVector objTran = disp->GetObjectTranslation() ; 
    objTran.setZ( objTran.z() + zoffset ); 
    disp->SetObjectTranslation( objTran ); 
}


/**
ZSolid::dumpUp
----------------

Ordinary postorder recursive traverse in order to get to all nodes. 
This approach should allow to obtain combination transforms in
complex trees. 

**/

void ZSolid::dumpUp(const char* msg) const 
{
    assert( parent_map ); 
    std::cout << msg << std::endl ; 
    dumpUp_r(root, 0); 
}

void ZSolid::dumpUp_r(const G4VSolid* node, int depth) const  
{
    if(Boolean(node))
    {
        dumpUp_r(ZSolid::Left(  node ), depth+1 ); 
        dumpUp_r(ZSolid::Right( node ), depth+1 ); 
    }
    else
    {
        G4RotationMatrix tree_rot ; 
        G4ThreeVector    tree_tla(0., 0., 0. ); 
        getTreeTransform(&tree_rot, &tree_tla, node ); 

        const G4VSolid* nd = Moved(nullptr, nullptr, node); 
        std::cout 
            << "ZSolid::dumpUp_r" 
            << " depth " << depth 
            << " type " << std::setw(20) << EntityTypeName(nd) 
            << " name " << std::setw(20) << nd->GetName() 
            << " tree_tla (" 
            << std::fixed << std::setw(7) << std::setprecision(2) << tree_tla.x() << " " 
            << std::fixed << std::setw(7) << std::setprecision(2) << tree_tla.y() << " "
            << std::fixed << std::setw(7) << std::setprecision(2) << tree_tla.z() 
            << ")"
            << std::endl
            ; 
   }
}


/**
ZSolid::getTreeTransform
-------------------------------

Would normally use parent links to determine all transforms relevant to a node, 
but Geant4 boolean trees do not have parent links. 
Hence use an external parent_map to provide uplinks enabling iteration 
up the tree from any node up to the root. 

**/

void ZSolid::getTreeTransform( G4RotationMatrix* rot, G4ThreeVector* tla, const G4VSolid* node ) const 
{
    const G4VSolid* nd = node ; 

    unsigned count = 0 ; 
    while(nd)
    {
        G4RotationMatrix r ; 
        G4ThreeVector    t(0., 0., 0. ); 
        const G4VSolid* dn = Moved( &r, &t, nd ); 
        assert( r.isIdentity() );  // simplifying assumption 

        *tla += t ;    // add up the translations 

        if(false) std::cout
            << "ZSolid::getTreeTransform" 
            << " count " << std::setw(2) << count 
            << " dn.name " << std::setw(20) << dn->GetName()
            << " dn.type " << std::setw(20) << dn->GetEntityType()
            << " dn.t (" 
            << std::fixed << std::setw(7) << std::setprecision(2) << t.x() << " " 
            << std::fixed << std::setw(7) << std::setprecision(2) << t.y() << " "
            << std::fixed << std::setw(7) << std::setprecision(2) << t.z() 
            << ")"
            << " tla (" 
            << std::fixed << std::setw(7) << std::setprecision(2) << tla->x() << " " 
            << std::fixed << std::setw(7) << std::setprecision(2) << tla->y() << " "
            << std::fixed << std::setw(7) << std::setprecision(2) << tla->z() 
            << ")"
            << std::endl 
            ; 

        nd = (*parent_map)[nd] ; // parentmap lineage uses G4DisplacedSolid so not using *dn* here
        count += 1 ; 
    }     
}




/**
ZSolid::DeepClone
-------------------

Clones a CSG tree of solids, assuming that the tree is
composed only of the limited set of primitives that are supported. 

G4BooleanSolid copy ctor just steals constituent pointers so 
it does not make an independent copy.  
Unlike the primitive copy ctors (at least those looked at: G4Polycone, G4Tubs) 
which appear to make properly independent copies 

**/

G4VSolid* ZSolid::DeepClone( const  G4VSolid* solid )  // static 
{
    G4RotationMatrix rot ; 
    G4ThreeVector tla ; 
    int depth = 0 ; 
    return DeepClone_r(solid, depth, &rot, &tla );  
}

/**
ZSolid::DeepClone_r
--------------------

G4DisplacedSolid is a wrapper for the for the right hand side of boolean constituent 
of a boolean combination which serves the purpose of holding the transform. 
The G4DisplacedSolid is automatically created by the G4BooleanSolid ctor when there is
an associated transform.  

The below *rot* and *tla* look at first glance like they are not used. 
But look more closely, the recursive DeepClone_r calls within BooleanClone are using them 
across the generations. This structure is necessary for BooleanClone because the 
transform from the child is needed when cloning the parent.

**/

G4VSolid* ZSolid::DeepClone_r( const G4VSolid* node_, int depth, G4RotationMatrix* rot, G4ThreeVector* tla )  // static 
{
    const G4VSolid* node = Moved( rot, tla, node_ ); 

    if(false) std::cout 
        << "ZSolid::DeepClone_r(preorder visit)"
        << " type " << std::setw(20) << EntityTypeName(node)
        << " name " << std::setw(20) << node->GetName()
        << " depth " << std::setw(2) << depth
        << " tla (" << tla->x() << " " << tla->y() << " " << tla->z() << ")" 
        << std::endl 
        ; 

    G4VSolid* clone = Boolean(node) ? BooleanClone(node, depth, rot, tla ) : PrimitiveClone(node) ; 
    assert(clone);
    return clone ; 
}    

G4VSolid* ZSolid::BooleanClone( const  G4VSolid* solid, int depth, G4RotationMatrix* rot, G4ThreeVector* tla ) // static
{
    G4String name = solid->GetName() ; 
    G4RotationMatrix lrot, rrot ;  
    G4ThreeVector    ltra, rtra ; 

    const G4BooleanSolid* src_boolean = dynamic_cast<const G4BooleanSolid*>(solid) ; 
    G4VSolid* left  = DeepClone_r( src_boolean->GetConstituentSolid(0), depth+1, &lrot, &ltra ) ; 
    G4VSolid* right = DeepClone_r( src_boolean->GetConstituentSolid(1), depth+1, &rrot, &rtra ) ; 

    assert( dynamic_cast<const G4DisplacedSolid*>(left) == nullptr  ) ; // not expecting these to be displaced 
    assert( dynamic_cast<const G4DisplacedSolid*>(right) == nullptr ) ; 
    assert( lrot.isIdentity() );   // lrot is expected to always be identity, as G4 never has left transforms
    assert( ltra.x() == 0. && ltra.y() == 0. && ltra.z() == 0. );  // not expecting transforms on the left
    assert( rrot.isIdentity() );   // rrot identity is a simplifying assumption

    G4VSolid* clone = nullptr ; 
    switch(EntityType(solid))
    {
        case _G4UnionSolid        : clone = new G4UnionSolid(       name, left, right, &rrot, rtra ) ; break ; 
        case _G4SubtractionSolid  : clone = new G4SubtractionSolid( name, left, right, &rrot, rtra ) ; break ;
        case _G4IntersectionSolid : clone = new G4IntersectionSolid(name, left, right, &rrot, rtra ) ; break ; 
    } 
    CheckBooleanClone( clone, left, right ); 
    return clone ; 
}

void ZSolid::CheckBooleanClone( const G4VSolid* clone, const G4VSolid* left, const G4VSolid* right ) // static
{
    if(!clone) std::cout << "ZSolid::CheckBooleanClone FATAL " << std::endl ; 
    assert(clone); 
    const G4BooleanSolid* boolean = dynamic_cast<const G4BooleanSolid*>(clone) ; 

    // lhs is never wrapped in G4DisplacedSolid 
    const G4VSolid* lhs = boolean->GetConstituentSolid(0) ; 
    const G4DisplacedSolid* lhs_disp = dynamic_cast<const G4DisplacedSolid*>(lhs) ; 
    assert( lhs_disp == nullptr && lhs == left ) ;      

    // rhs will be wrapped in G4DisplacedSolid as above G4BooleanSolid ctor has transform rrot/rtla
    const G4VSolid* rhs = boolean->GetConstituentSolid(1) ; 
    const G4DisplacedSolid* rhs_disp = dynamic_cast<const G4DisplacedSolid*>(rhs) ; 
    assert( rhs_disp != nullptr && rhs != right);    
    const G4VSolid* right_check = rhs_disp->GetConstituentMovedSolid() ;
    assert( right_check == right );  
}


G4VSolid* ZSolid::PrimitiveClone( const  G4VSolid* solid )  // static 
{
    G4VSolid* clone = nullptr ; 
    int type = EntityType(solid); 
    if( type == _G4Ellipsoid )
    {
        const G4Ellipsoid* ellipsoid = dynamic_cast<const G4Ellipsoid*>(solid) ; 
        clone = new G4Ellipsoid(*ellipsoid) ;
    }
    else if( type == _G4Tubs )
    {
        const G4Tubs* tubs = dynamic_cast<const G4Tubs*>(solid) ; 
        clone = new G4Tubs(*tubs) ;  
    }
    else if( type == _G4Polycone )
    {
        const G4Polycone* polycone = dynamic_cast<const G4Polycone*>(solid) ; 
        clone = new G4Polycone(*polycone) ;  
    }
    else
    {
        std::cout 
            << "ZSolid::PrimitiveClone FATAL unimplemented prim type " << EntityTypeName(solid) 
            << std::endl 
            ;
        assert(0); 
    } 
    return clone ; 
}

double ZSolid::getZ( const G4VSolid* node ) const
{
    G4RotationMatrix tree_rot ; 
    G4ThreeVector    tree_tla(0., 0., 0. ); 
    getTreeTransform(&tree_rot, &tree_tla, node ); 

    double zdelta = tree_tla.z() ; 
    return zdelta ; 
}

bool ZSolid::CanZ( const G4VSolid* solid ) // static
{
    int type = EntityType(solid) ; 
    return type == _G4Ellipsoid || type == _G4Tubs || type == _G4Polycone ; 
}
void ZSolid::ZRange( double& z0, double& z1, const G4VSolid* solid) // static  
{
    switch(EntityType(solid))
    {
        case _G4Ellipsoid: GetZRange( dynamic_cast<const G4Ellipsoid*>(solid), z0, z1 );  break ; 
        case _G4Tubs:      GetZRange( dynamic_cast<const G4Tubs*>(solid)    ,  z0, z1 );  break ; 
        case _G4Polycone:  GetZRange( dynamic_cast<const G4Polycone*>(solid),  z0, z1 );  break ; 
        case _G4Other:    { std::cout << "ZSolid::GetZ FATAL : not implemented for entityType " << EntityTypeName(solid) << std::endl ; assert(0) ; } ; break ;  
    }
}

void ZSolid::GetZRange( const G4Ellipsoid* const ellipsoid, double& _z0, double& _z1 )  // static 
{
    _z1 = ellipsoid->GetZTopCut() ; 
    _z0 = ellipsoid->GetZBottomCut() ;  
}
void ZSolid::GetZRange( const G4Tubs* const tubs, double& _z0, double& _z1 )  // static 
{
    _z1 = tubs->GetZHalfLength() ;  
    _z0 = -_z1 ;  
    assert( _z1 > 0. ); 
}
void ZSolid::GetZRange( const G4Polycone* const polycone, double& _z0, double& _z1 )  // static 
{
    G4PolyconeHistorical* pars = polycone->GetOriginalParameters(); 
    unsigned num_z = pars->Num_z_planes ; 
    for(unsigned i=1 ; i < num_z ; i++)
    {
        double z0 = pars->Z_values[i-1] ; 
        double z1 = pars->Z_values[i] ; 
        assert( z1 > z0 );   
    }
    _z1 = pars->Z_values[num_z-1] ; 
    _z0 = pars->Z_values[0] ;  
}

