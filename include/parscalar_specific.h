// -*- C++ -*-
// $Id: parscalar_specific.h,v 1.9 2003-01-16 03:35:57 edwards Exp $
//
// QDP data parallel interface
//
// Outer lattice routines specific to a parallel platform with scalar layout

#include "QMP.h"

QDP_BEGIN_NAMESPACE(QDP);

// Use separate defs here. This will cause subroutine calls under g++

//-----------------------------------------------------------------------------
// Layout stuff specific to a parallel architecture
namespace Layout
{
  //! Returns the logical node coordinates for this node
  const multi1d<int>& nodeCoord();

  //! Returns the logical size of this machine
  const multi1d<int>& logicalSize();

  //! Returns the logical node coordinates for the corresponding lattice coordinate
  /*! The API requires this function to be here */
  multi1d<int> nodeCoord(const multi1d<int>& coord);

  //! Subgrid (grid on each node) lattice size
  const multi1d<int>& subgridLattSize();

  //! coord[mu]  <- mu  : fill with lattice coord in mu direction
  LatticeInteger latticeCoordinate(int mu);
};


//-----------------------------------------------------------------------------
// Internal ops with ties to QMP
namespace Internal
{
  //! Slow send-receive (blocking)
  void sendRecvWait(void *send_buf, void *recv_buf, 
		    int count, int isign0, int dir);

  //! Send-receive routine
  void sendRecv(void *send_buf, void *recv_buf, 
		int count, int isign0, int dir);

  //! Wait on send-receive
  void wait(int dir);

  //! Send to another node (wait)
  void sendToWait(void *send_buf, int dest_node, int count);

  //! Receive from another node (wait)
  void recvFromWait(void *recv_buf, int srce_node, int count);

  //! Via some mechanism, get the dest to node 0
  /*! Ultimately, I do not want to use point-to-point */
  template<class T>
  void sendToPrimaryNode(T& dest, int srcnode)
  {
    if (srcnode != 0)
    {
      if (Layout::primaryNode())
	recvFromWait((void *)&dest, srcnode, sizeof(T));

      if (Layout::nodeNumber() == srcnode)
	sendToWait((void *)&dest, 0, sizeof(T));
    }
  }

  //! Sum across all nodes
  template<class T>
  void global_sum(T& dest)
  {
//    QMP_global_sum((void *)&dest, sizeof(T));
  }

  //! Broadcast from primary node to all other nodes
  template<class T>
  void broadcast(T& dest)
  {
    QMP_broadcast((void *)&dest, sizeof(T));
  }

};



//-----------------------------------------------------------------------------
//! OLattice Op Scalar(Expression(source)) under a subset
/*! 
 * OLattice Op Expression, where Op is some kind of binary operation 
 * involving the destination 
 */
template<class T, class T1, class Op, class RHS>
//inline
void evaluate(OLattice<T>& dest, const Op& op, const QDPExpr<RHS,OScalar<T1> >& rhs,
	      const Subset& s)
{
//  cerr << "In evaluate(olattice,oscalar)\n";

  const int *tab = s.SiteTable()->slice();
  for(int j=0; j < s.NumSiteTable(); ++j) 
  {
    int i = tab[j];
//    fprintf(stderr,"eval(olattice,oscalar): site %d\n",i);
//    op(dest.elem(i), forEach(rhs, ElemLeaf(), OpCombine()));
    op(dest.elem(i), forEach(rhs, EvalLeaf1(0), OpCombine()));
  }
}

//! OLattice Op Scalar(Expression(source)) under the ALL subset
/*! 
 * OLattice Op Expression, where Op is some kind of binary operation 
 * involving the destination 
 *
 * For now, always goes through the ALL subset
 * This helps with simplifying code development, but eventually
 * should be specific to the all subset
 */
template<class T, class T1, class Op, class RHS>
inline
void evaluate(OLattice<T>& dest, const Op& op, const QDPExpr<RHS,OScalar<T1> >& rhs)
{
  evaluate(dest, op, rhs, all);
}


//! OLattice Op OLattice(Expression(source)) under a subset
/*! 
 * OLattice Op Expression, where Op is some kind of binary operation 
 * involving the destination 
 */
template<class T, class T1, class Op, class RHS>
//inline
void evaluate(OLattice<T>& dest, const Op& op, const QDPExpr<RHS,OLattice<T1> >& rhs,
	      const Subset s)
{
//  cerr << "In evaluate(olattice,olattice)\n";

// NOTE: this code below is the first way I did the loop. The
// case when IndexRep is false is the optimal loop structure
// However, for simplicity and maintenance I will use the general
// form for all methods

//   if (! s.IndexRep())
//     for(int i=s.Start(); i <= s.End(); ++i) 
//     {
//       op(dest.elem(i), forEach(rhs, EvalLeaf1(0), OpCombine()));
//     }
//   else
//   {
//     const int *tab = s.SiteTable()->slice();
//     for(int j=0; j < s.NumSiteTable(); ++j) 
//     {
//       int i = tab[j];
//       op(dest.elem(i), forEach(rhs, EvalLeaf1(0), OpCombine()));
//     }
//   }
 
  // General form of loop structure
  const int *tab = s.SiteTable()->slice();
  for(int j=0; j < s.NumSiteTable(); ++j) 
  {
    int i = tab[j];
//    fprintf(stderr,"eval(olattice,olattice): site %d\n",i);
    op(dest.elem(i), forEach(rhs, EvalLeaf1(i), OpCombine()));
  }
}


//! OLattice Op OLattice(Expression(source))
template<class T, class T1, class Op, class RHS>
inline
void evaluate(OLattice<T>& dest, const Op& op, const QDPExpr<RHS,OLattice<T1> >& rhs)
{
  // Use the general loop form. However, could have an explicit loop
  // with no index lookup.
  evaluate(dest, op, rhs, all);
}



//-----------------------------------------------------------------------------
//! dest = (mask) ? s1 : dest
template<class T1, class T2> 
void copymask(OSubLattice<T2> d, const OLattice<T1>& mask, const OLattice<T2>& s1) 
{
  OLattice<T2>& dest = d.field();
  const Subset& s = d.subset();

  const int *tab = s.SiteTable()->slice();
  for(int j=0; j < s.NumSiteTable(); ++j) 
  {
    int i = tab[j];
    copymask(dest.elem(i), mask.elem(i), s1.elem(i));
  }
}

//! dest = (mask) ? s1 : dest
template<class T1, class T2> 
void copymask(OLattice<T2>& dest, const OLattice<T1>& mask, const OLattice<T2>& s1) 
{
  for(int i=0; i < Layout::subgridVol(); ++i) 
    copymask(dest.elem(i), mask.elem(i), s1.elem(i));
}


//-----------------------------------------------------------------------------
// Random numbers
namespace RNG
{
  extern Seed ran_seed;
  extern Seed ran_mult;
  extern Seed ran_mult_n;
  extern LatticeSeed *lattice_ran_mult;
};


//! dest  = random  
/*! This implementation is correct for no inner grid */
template<class T>
void random(OScalar<T>& d)
{
  Seed seed = RNG::ran_seed;
  Seed skewed_seed = RNG::ran_seed * RNG::ran_mult;

  fill_random(d.elem(), seed, skewed_seed, RNG::ran_mult);

  RNG::ran_seed = seed;  // The seed from any site is the same as the new global seed
}


//! dest  = random    under a subset
template<class T>
void random(OSubLattice<T> dd)
{
  OLattice<T>& d = dd.field();
  const Subset& s = dd.subset();

  Seed seed;
  Seed skewed_seed;

  const int *tab = s.SiteTable()->slice();
  for(int j=0; j < s.NumSiteTable(); ++j) 
  {
    int i = tab[j];
    seed = RNG::ran_seed;
    skewed_seed.elem() = RNG::ran_seed.elem() * RNG::lattice_ran_mult->elem(i);
    fill_random(d.elem(i), seed, skewed_seed, RNG::ran_mult_n);
  }

  RNG::ran_seed = seed;  // The seed from any site is the same as the new global seed
}


//! dest  = random  
template<class T>
void random(OLattice<T>& d)
{
  random(d(all));
}


//! dest  = random   under a subset
template<class T>
void gaussian(OSubLattice<T> dd)
{
  OLattice<T>& d = dd.field();
  const Subset& s = dd.subset();

  OLattice<T>  r1, r2;

  random(r1(s));
  random(r2(s));

  const int *tab = s.SiteTable()->slice();
  for(int j=0; j < s.NumSiteTable(); ++j) 
  {
    int i = tab[j];
    fill_gaussian(d.elem(i), r1.elem(i), r2.elem(i));
  }
}


//! dest  = random  
template<class T>
void gaussian(OLattice<T>& d)
{
  gaussian(d(all));
}



//-----------------------------------------------------------------------------
// Broadcast operations
//! dest  = 0 
template<class T> 
inline
void zero_rep(OLattice<T>& dest, const Subset& s) 
{
  const int *tab = s.SiteTable()->slice();
  for(int j=0; j < s.NumSiteTable(); ++j) 
  {
    int i = tab[j];
    zero_rep(dest.elem(i));
  }
}


//! dest  = 0 
template<class T> 
void zero_rep(OSubLattice<T> dd) 
{
  OLattice<T>& d = dd.field();
  const Subset& s = dd.subset();
  
  zero_rep(d,s);
}


//! dest  = 0 
template<class T> 
void zero_rep(OLattice<T>& dest) 
{
  for(int i=0; i < Layout::subgridVol(); ++i) 
    zero_rep(dest.elem(i));
}



//-----------------------------------------------
// Global sums
//! OScalar = sum(OScalar) under an explicit subset
/*!
 * Allow a global sum that sums over the lattice, but returns an object
 * of the same primitive type. E.g., contract only over lattice indices
 */
template<class RHS, class T>
typename UnaryReturn<OScalar<T>, FnSum>::Type_t
sum(const QDPExpr<RHS,OScalar<T> >& s1, const Subset& s)
{
  typename UnaryReturn<OScalar<T>, FnSum>::Type_t  d;

  evaluate(d,OpAssign(),s1);   // since OScalar, no global sum needed
  return d;
}


//! OScalar = sum(OScalar)
/*!
 * Allow a global sum that sums over the lattice, but returns an object
 * of the same primitive type. E.g., contract only over lattice indices
 */
template<class RHS, class T>
typename UnaryReturn<OScalar<T>, FnSum>::Type_t
sum(const QDPExpr<RHS,OScalar<T> >& s1)
{
  typename UnaryReturn<OScalar<T>, FnSum>::Type_t  d;

  evaluate(d,OpAssign(),s1);   // since OScalar, no global sum needed
  return d;
}



//! OScalar = sum(OLattice)  under an explicit subset
/*!
 * Allow a global sum that sums over the lattice, but returns an object
 * of the same primitive type. E.g., contract only over lattice indices
 */
template<class RHS, class T>
typename UnaryReturn<OLattice<T>, FnSum>::Type_t
sum(const QDPExpr<RHS,OLattice<T> >& s1, const Subset& s)
{
  typename UnaryReturn<OLattice<T>, FnSum>::Type_t  d;

  // Must initialize to zero since we do not know if the loop will be entered
  zero_rep(d.elem());

  const int *tab = s.SiteTable()->slice();
  for(int j=0; j < s.NumSiteTable(); ++j) 
  {
    int i = tab[j];
    d.elem() += forEach(s1, EvalLeaf1(i), OpCombine());   // SINGLE NODE VERSION FOR NOW
  }

  // Do a global sum on the result
  Internal::global_sum(d);
  
  return d;
}


//! OScalar = sum(OLattice)
/*!
 * Allow a global sum that sums over the lattice, but returns an object
 * of the same primitive type. E.g., contract only over lattice indices
 */
template<class RHS, class T>
typename UnaryReturn<OLattice<T>, FnSum>::Type_t
sum(const QDPExpr<RHS,OLattice<T> >& s1)
{
  typename UnaryReturn<OLattice<T>, FnSum>::Type_t  d;

  // Loop always entered - could unroll
  zero_rep(d.elem());

  for(int i=0; i < Layout::subgridVol(); ++i) 
    d.elem() += forEach(s1, EvalLeaf1(i), OpCombine());

  // Do a global sum on the result
  Internal::global_sum(d);
  
  return d;
}


//-----------------------------------------------------------------------------
// Multiple global sums 
//! multi1d<OScalar> dest  = sumMulti(OScalar,Set) 
/*!
 * Compute the global sum on multiple subsets specified by Set 
 *
 * This implementation is specific to a purely olattice like
 * types. The scalar input value is replicated to all the
 * slices
 */
template<class RHS, class T>
typename UnaryReturn<OScalar<T>, FnSum>::Type_t
sumMulti(const QDPExpr<RHS,OScalar<T> >& s1, const Set& ss)
{
  typename UnaryReturn<OScalar<T>, FnSumMulti>::Type_t  dest(ss.numSubsets());

  // lazy - evaluate repeatedly
  for(int i=0; i < ss.numSubsets(); ++i)
  {
    evaluate(dest[i],OpAssign(),s1);
  }

  return dest;
}


//! multi1d<OScalar> dest  = sumMulti(OLattice,Set) 
/*!
 * Compute the global sum on multiple subsets specified by Set 
 *
 * This is a very simple implementation. There is no need for
 * anything fancier unless global sums are just so extraordinarily
 * slow. Otherwise, generalized sums happen so infrequently the slow
 * version is fine.
 */
template<class RHS, class T>
typename UnaryReturn<OLattice<T>, FnSumMulti>::Type_t
sumMulti(const QDPExpr<RHS,OLattice<T> >& s1, const Set& ss)
{
  typename UnaryReturn<OLattice<T>, FnSumMulti>::Type_t  dest(ss.numSubsets());

  // Initialize result with zero
  for(int k=0; k < ss.numSubsets(); ++k)
    zero_rep(dest[k]);

  // Loop over all sites and accumulate based on the coloring 
  const multi1d<int>& lat_color =  ss.LatticeColoring();

  for(int i=0; i < Layout::subgridVol(); ++i) 
  {
    int j = lat_color[i];
    dest[j].elem() += forEach(s1, EvalLeaf1(i), OpCombine());   // SINGLE NODE VERSION FOR NOW
  }

  // Do a global sum on the result
  for(int k=0; k < ss.numSubsets(); ++k)
    Internal::global_sum(dest[k]);
  
  return dest;
}


//-----------------------------------------------------------------------------
// Peek and poke at individual sites. This is very architecture specific
// NOTE: these two routines assume there is no underlying inner grid

//! Extract site element
/*! @ingroup group1
  @param l  source to examine
  @param coord Nd lattice coordinates to examine
  @return single site object of the same primitive type
  @ingroup group1
  @relates QDPType */
template<class T1>
inline OScalar<T1>
peekSite(const OScalar<T1>& l, const multi1d<int>& coord)
{
  return l;
}


//! Extract site element
/*! @ingroup group1
  @param l  source to examine
  @param coord Nd lattice coordinates to examine
  @return single site object of the same primitive type
  @ingroup group1
  @relates QDPType */
template<class T1>
inline OScalar<T1>
peekSite(const OLattice<T1>& l, const multi1d<int>& coord)
{
  OScalar<T1> dest;
  int nodenum = Layout::nodeNumber(coord);

  // Find the result somewhere within the machine.
  // Then we must get it to node zero so we can broadcast it
  // out to all nodes
  if (Layout::nodeNumber() == nodenum)
    dest.elem() = l.elem(Layout::linearSiteIndex(coord));
  else
    zero_rep(dest.elem());

  // Send result to primary node via some mechanism
  Internal::sendToPrimaryNode(dest, nodenum);

  // Now broadcast back out to all nodes
  Internal::broadcast(dest);

  return dest;
}


//! Insert site element
/*! @ingroup group1
  @param l  target to update
  @param r  source to insert
  @param coord Nd lattice coordinates where to insert
  @return object of the same primitive type but of promoted lattice type
  @ingroup group1
  @relates QDPType */
template<class T1>
inline OLattice<T1>&
pokeSite(OLattice<T1>& l, const OScalar<T1>& r, const multi1d<int>& coord)
{
  if (Layout::nodeNumber() == Layout::nodeNumber(coord))
    l.elem(Layout::linearSiteIndex(coord)) = r.elem();

  return l;
}



//-----------------------------------------------------------------------------
// Map
//! General permutation map class for communications */
class Map
{
public:
  //! Constructor - does nothing really
  Map() {}

  //! Destructor
  ~Map() {}

  //! Constructor from a function object
  Map(const MapFunc& fn) {make(fn);}

  //! Actual constructor from a function object
  /*! The semantics are   source_site = func(dest_site) */
  void make(const MapFunc& func);

  //! Function call operator for a shift
  /*! 
   * map(source)
   *
   * Implements:  dest(x) = s1(x+offsets)
   *
   * Shifts on a OLattice are non-trivial.
   * Notice, there may be an ILattice underneath which requires shift args.
   * This routine is very architecture dependent.
   */
  template<class T1,class C1>
  QDPType<T1,C1>
  operator()(const QDPType<T1,C1> & l)
    {
      QMP_info("Map()");

      QDPType<T1,C1> dd;
      C1& d = static_cast<C1&>(const_cast<QDPType<T1,C1>&>(dd));

      if (offnodeP)
      {
	// Off-node communications required
	QMP_info("Map: off-node communications required");

	// Eventually these declarations should move into d - the return object
	typedef T1 * T1ptr;
	T1 **dest = new T1ptr[Layout::subgridVol()];
	QMP_msgmem_t msg[2];
	QMP_msghandle_t mh_a[2], mh;

	// For now, will assume there is only 1 destination node 
	// and receive from only 1 node
	if (srcenodes.size() != 1)
	  QMP_error_exit("Map: for now only allow 1 destination node");
      
	if (destnodes.size() != 1)
	  QMP_error_exit("Map: for now only allow receives from 1 node");
      
	int dstnum = destnodes_num[0]*sizeof(T1);
	int srcnum = srcenodes_num[0]*sizeof(T1);
	T1 *send_buf = (T1 *)(QMP_allocate_aligned_memory(dstnum)); // packed data to send
	T1 *recv_buf = (T1 *)(QMP_allocate_aligned_memory(srcnum)); // packed receive data

	// Gather the face of data to send
	// For now, use the all subset
	const int my_node = Layout::nodeNumber();
	for(int i=0, si=0, ri=0; i < Layout::subgridVol(); ++i) 
	{
	  if (dstnode[i] != my_node)
	    send_buf[si++] = l.elem(i);

	  if (srcnode[i] != my_node)
	  {
	    QMP_info("Map_gather_send(olattice[%d],olattice[%d])",i,ri);

	    dest[i] = &(recv_buf[ri++]);
	  }
	  else
	  {
	    QMP_info("Map_gather_onnode(olattice[%d],olattice[%d])",i,soffsets[i]);

	    dest[i] = &(const_cast<T1&>(l.elem(soffsets[i])));
	  }
	}

	QMP_status_t err;

	QMP_info("Map: send = 0x%x  recv = 0x%x",send_buf,recv_buf);

	msg[0]  = QMP_declare_msgmem(recv_buf, srcnum);
	msg[1]  = QMP_declare_msgmem(send_buf, dstnum);
	mh_a[0] = QMP_declare_receive_from(msg[0], srcenodes[0], 0);
	mh_a[1] = QMP_declare_send_to(msg[1], destnodes[0], 0);
	mh      = QMP_declare_multiple(mh_a, 2);

	// Launch the faces
	if ((err = QMP_start(mh)) != QMP_SUCCESS)
	  QMP_error_exit(QMP_error_string(err));

	// Wait on the faces
	if ((err = QMP_wait(mh)) != QMP_SUCCESS)
	  QMP_error_exit(QMP_error_string(err));

	QMP_free_msghandle(mh_a[1]);
	QMP_free_msghandle(mh_a[0]);
	QMP_free_msghandle(mh);
	QMP_free_msgmem(msg[1]);
	QMP_free_msgmem(msg[0]);

	// Scatter the data into the destination
	// Some of the data maybe in receive buffers
	// For now, use the all subset
	for(int i=0; i < Layout::subgridVol(); ++i) 
	{
	  QMP_info("Map_scatter(olattice[%d],olattice[0x%x])",i,dest[i]);
	  d.elem(i) = *(dest[i]);
	}

	QMP_info("finished scatter");

	// Cleanup
	QMP_free_aligned_memory(recv_buf);
	QMP_free_aligned_memory(send_buf);
	delete dest;

	QMP_info("finished cleanup");
      }
      else 
      {
	// No off-node communications - copy on node
	QMP_info("Map: copy on node - no communications");

	// For now, use the all subset
	for(int i=0; i < Layout::subgridVol(); ++i) 
	{
	  QMP_info("Map(olattice[%d],olattice[%d])",i,soffsets[i]);
	  d.elem(i) = l.elem(soffsets[i]);
	}
      }


      QMP_info("exiting Map()");

      return dd;
    }


  template<class RHS, class T1>
  QDPType<T1,OScalar<T1> >
  operator()(const QDPExpr<RHS,OScalar<T1> > & l)
    {
      // For now, simply evalute the expression and then do the map
      typedef OScalar<T1> C1;
      QDPType<T1,C1> d = this->operator()(C1(l));

      fprintf(stderr,"map(QDPExpr)\n");
      return d;
    }

  template<class RHS, class T1>
  QDPType<T1,OLattice<T1> >
  operator()(const QDPExpr<RHS,OLattice<T1> > & l)
    {
      // For now, simply evalute the expression and then do the map
      typedef OLattice<T1> C1;
      QDPType<T1,C1> d = this->operator()(C1(l));

      fprintf(stderr,"map(QDPExpr)\n");
      return d;
    }


public:
  //! Accessor to offsets
  const multi1d<int>& offsets() const {return soffsets;}

private:
  //! Hide copy constructor
  Map(const Map&) {}

  //! Hide operator=
  void operator=(const Map&) {}

private:
  //! Offset table used for communications. 
  /*! 
   * The direction is in the sense of the Map or Shift functions from QDP.
   * soffsets(position) 
   */ 
  multi1d<int> soffsets;
  multi1d<int> srcnode;
  multi1d<int> dstnode;

  multi1d<int> srcenodes;
  multi1d<int> destnodes;

  multi1d<int> srcenodes_num;
  multi1d<int> destnodes_num;

  // Indicate off-node communications is needed;
  bool offnodeP;
};


//-----------------------------------------------------------------------------
//
// This is the PETE version of a shift, namely return an expression
//
// This class looks like those in OperatorTags, but has a specific constructor
// for a given direction
// This mechanism needs to be more general - this implementation is a prototype.
//
// NOTE: the use of "all" is not desired. The offsets is not suppose to
// be subset dependent, but is general to the class. E.g., all shifts should be
// static classes
class NearestNeighborMap
{
public:
  //! Constructor - does nothing really
  NearestNeighborMap() {}

  //! Destructor
  ~NearestNeighborMap() {}

  //! Actual constructor
  void make();

  //! Function call operator for a shift
  /*! 
   * shift(source,isign,dir)
   * isign = parity of direction (+1 or -1)
   * dir   = direction ([0,...,Nd-1])
   *
   * Implements:  dest(x) = s1(x+isign*dir)
   * There are cpp macros called  FORWARD and BACKWARD that are +1,-1 resp.
   * that are often used as arguments
   *
   * Shifts on a OLattice are non-trivial.
   * Notice, there may be an ILattice underneath which requires shift args.
   * This routine is very architecture dependent.
   */
  template<class T1,class C1>
  inline typename MakeReturn<UnaryNode<OpIdentity,
    typename CreateLeaf<QDPType<T1,C1> >::Leaf_t>,
    typename UnaryReturn<C1,OpIdentity >::Type_t >::Expression_t
  operator()(const QDPType<T1,C1> & l, int isign, int dir)
    {
      fprintf(stderr,"shift(QDPType,%d,%d)\n",isign,dir);

      typedef UnaryNode<OpIdentity,
	typename CreateLeaf<QDPType<T1,C1> >::Leaf_t> Tree_t;
      typedef typename UnaryReturn<C1,OpIdentity >::Type_t Container_t;
      return MakeReturn<Tree_t,Container_t>::make(Tree_t(
	CreateLeaf<QDPType<T1,C1> >::make(l)));
    }


  template<class T1,class C1>
  inline typename MakeReturn<UnaryNode<OpIdentity,
    typename CreateLeaf<QDPExpr<T1,C1> >::Leaf_t>,
    typename UnaryReturn<C1,OpIdentity >::Type_t >::Expression_t
  operator()(const QDPExpr<T1,C1> & l, int isign, int dir)
    {
      fprintf(stderr,"shift(QDPExpr,%d,%d)\n",isign,dir);

      typedef UnaryNode<OpIdentity,
	typename CreateLeaf<QDPExpr<T1,C1> >::Leaf_t> Tree_t;
      typedef typename UnaryReturn<C1,OpIdentity >::Type_t Container_t;
      return MakeReturn<Tree_t,Container_t>::make(Tree_t(
	CreateLeaf<QDPExpr<T1,C1> >::make(l)));
    }


public:
  //! Accessor to offsets
  const multi3d<int>& Offsets() const {return soffsets;}

private:
  //! Hide copy constructor
  NearestNeighborMap(const NearestNeighborMap&) {}

  //! Hide operator=
  void operator=(const NearestNeighborMap&) {}

private:
  //! Offset table used for communications. 
  /*! 
   * The direction is in the sense of the Map or Shift functions from QDP.
   * soffsets(direction,isign,position) 
   */ 
  multi3d<int> soffsets;
  multi3d<int> srcnode;
  multi3d<int> dstnode;

  multi2d<int> srcenodes_num;
  multi2d<int> dstenodes_num;
};




//-----------------------------------------------------------------------------
extern "C"
{
  extern int QMP_shift(int site, unsigned char *data, int prim_size, int sn);
}

//! Ascii output
/*! Assumes no inner grid */
template<class T>  
NmlWriter& operator<<(NmlWriter& nml, const OLattice<T>& d)
{
  if (Layout::primaryNode())
    nml.get() << "   [OUTER]" << endl;

  // Twice the subgrid vol - intermediate array we flip-flop writing data
  multi1d<T> data(2*Layout::subgridVol());

  for(int site=0; site < Layout::subgridVol(); ++site)
    data[site] = d.elem(site);

  const int xinc = Layout::subgridLattSize()[0];

  // Assume lexicographic for the moment...
  for(int site=0, xsite1=0; site < Layout::vol(); site += xinc)
  {
//    int i = Layout::linearSiteIndex(site);
    int xsite2 = QMP_shift(site,(unsigned char*)(data.slice()),sizeof(T),0);

    if (Layout::primaryNode())
      for(int xsite3=0; xsite3 < xinc; xsite3++,xsite2++,xsite1++)
      {
	nml.get() << "   Site =  " << xsite1 << "   = ";
	nml << data[xsite2];
	nml.get() << "," << endl;
      }
  }

  return nml;
}

//! Binary output
/*! Assumes no inner grid */
template<class T>
BinaryWriter& write(BinaryWriter& bin, const OLattice<T>& d)
{
  // Twice the subgrid vol - intermediate array we flip-flop writing data
  multi1d<T> data(2*Layout::subgridVol());

  for(int site=0; site < Layout::subgridVol(); ++site)
    data[site] = d.elem(site);

  const int xinc = Layout::subgridLattSize()[0];

  // Assume lexicographic for the moment...
  for(int site=0; site < Layout::vol(); site += xinc)
  {
//    int i = Layout::linearSiteIndex(site);
    int xsite2 = QMP_shift(site,(unsigned char*)(data.slice()),sizeof(T),0);

    if (Layout::primaryNode())
      bfwrite((void *)(data.slice() + xsite2), 
	      sizeof(typename WordType<T>::Type_t), 
	      xinc*sizeof(T)/sizeof(typename WordType<T>::Type_t), bin.get());
  }
  return bin;
}


//! Read a binary element
template<class T>
BinaryReader& read(BinaryReader& bin, T& d)
{
  if (Layout::primaryNode()) 
    if (bfread((void *)&d, sizeof(T), 1, bin.get()) != 1)
      QDP_error_exit("BinaryReader: failed to read");

  // Now broadcast back out to all nodes
  Internal::broadcast(d);

  return bin;
}

//! Binary input
/*! Assumes no inner grid */
template<class T>
BinaryReader& read(BinaryReader& bin, OScalar<T>& d)
{
  if (Layout::primaryNode()) 
    bfread((void *)&(d.elem()), sizeof(typename WordType<T>::Type_t), 
	   sizeof(T) / sizeof(typename WordType<T>::Type_t), bin.get()); 

  // Now broadcast back out to all nodes
  Internal::broadcast(d);

  return bin;
}

//! Binary input
/*! Assumes no inner grid */
template<class T>
BinaryReader& read(BinaryReader& bin, OLattice<T>& d)
{
  // Twice the subgrid vol - intermediate array we flip-flop reading data
  multi1d<T> data(2*Layout::subgridVol());
  const int xinc = Layout::subgridLattSize()[0];

  // Assume lexicographic for the moment...
  for(int site=0, xsite2=0; site < Layout::vol(); site += xinc)
  {
    if (Layout::primaryNode())
      bfread((void *)(data.slice() + xsite2),
	     sizeof(typename WordType<T>::Type_t), 
	     xinc*sizeof(T)/sizeof(typename WordType<T>::Type_t), bin.get());

    xsite2 = QMP_shift((site + xinc) % Layout::vol(),
		       (unsigned char*)(data.slice()), sizeof(T), xinc);
  }

  for(int site=0; site < Layout::subgridVol(); ++site)
  {
//    int i = Layout::linearSiteIndex(site);
    d.elem(site) = data[site];
  }

  return bin;
}

QDP_END_NAMESPACE();
