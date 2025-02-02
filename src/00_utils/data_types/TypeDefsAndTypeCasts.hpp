#ifndef __femus_utils_Casts_hpp__
#define __femus_utils_Casts_hpp__

//The appropriate includes...
#include <cassert>

#include <typeinfo>


// ===  Typedefs - BEGIN =================

typedef unsigned int uint;

// ===  Typedefs - END =================



namespace femus
{


//TODO
  template <typename Tnew, typename Told>
  inline Tnew cast_ptr(Told* oldvar)
  {
#ifndef NDEBUG
    Tnew newvar = dynamic_cast<Tnew>(oldvar);
    assert(newvar);
    return newvar;
#else
    return(static_cast<Tnew>(oldvar));
#endif
  }


//***********PORCHETTA
  template <typename Tnew, typename Told>
  inline Tnew cast_ref(Told& oldvar)
  {
#ifndef NDEBUG
    try {
      Tnew newvar = dynamic_cast<Tnew>(oldvar);
      return newvar;
    }
    catch(std::bad_cast) {
      assert(false);
    }
#else
    return(static_cast<Tnew>(oldvar));
#endif
  }
} //end namespace femus



#endif
