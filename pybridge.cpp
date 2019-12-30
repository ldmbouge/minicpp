#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <pybind11/functional.h>

#include <sstream>
#include "store.hpp"
#include "solver.hpp"
#include "avar.hpp"
#include "intvar.hpp"
#include "acstr.hpp"
#include "constraint.hpp"
#include "search.hpp"

namespace py = pybind11;

PYBIND11_DECLARE_HOLDER_TYPE(T, handle_ptr<T>,true);
PYBIND11_MAKE_OPAQUE(Factory::Veci);
                     
PYBIND11_MODULE(minicpp,m) {
   m.doc() = "MiniCPP Plugin for Python";

   py::register_exception_translator([](std::exception_ptr p) {
                                        try {
                                           if (p) std::rethrow_exception(p);
                                        } catch (Status e) {
                                           PyErr_SetString(PyExc_RuntimeError, "FAIL");
                                        }
                                     });

   //py::register_exception<Status>(m, "PyStatus");
   
   py::class_<Branches>(m,"Branches")
      .def(py::init<>())
      .def(py::init<std::function<void(void)>,std::function<void(void)>>(),py::keep_alive<1,2>());
   
   py::class_<Chooser>(m,"Chooser");
   py::class_<AVar,AVar::Ptr>(m,"AVar");
   py::class_<Constraint,Constraint::Ptr>(m,"Constraint")
      .def("post",&Constraint::post)
      .def("propagate",&Constraint::propagate)
      .def("setSchedule",&Constraint::setScheduled)
      .def("isScheduled",&Constraint::isScheduled)
      .def("setActive",&Constraint::setActive)
      .def("isActive",&Constraint::isActive)
      .def("__repr__",[](const Constraint& s) {
                         std::ostringstream str;
                         s.print(str);
                         str << std::ends;
                         return str.str();
                      });

   py::class_<Objective,Objective::Ptr>(m,"Objective")
      .def("tighten",&Objective::tighten)
      .def("value",&Objective::value);

   py::class_<var<int>,AVar,var<int>::Ptr>(m,"VarInt")
      .def_property_readonly("min",&var<int>::min,"The smallest value in the domain")
      .def_property_readonly("max",&var<int>::max)
      .def_property_readonly("size",&var<int>::size)
      .def_property_readonly("isBound",&var<int>::isBound)
      .def("contains",&var<int>::contains)
      .def("assign",&var<int>::assign)
      .def("remove",&var<int>::remove)
      .def("removeBelow",&var<int>::removeBelow)
      .def("removeAbove",&var<int>::removeAbove)
      .def("updateBounds",&var<int>::updateBounds)
      .def("__neg__",[](const var<int>::Ptr& s)        { return Factory::operator-(s);},py::is_operator())
      .def("__mul__",[](const var<int>::Ptr& s,int a)  { return Factory::operator*(s,a);},py::is_operator())
      .def("__rmul__",[](const var<int>::Ptr& s,int a) { return Factory::operator*(s,a);},py::is_operator())
      .def("__add__",[](const var<int>::Ptr& s,int a)  { return Factory::operator+(s,a);},py::is_operator())
      .def("__radd__",[](const var<int>::Ptr& s,int a) { return Factory::operator+(s,a);},py::is_operator())
      .def("__sub__",[](const var<int>::Ptr& s,int a)  { return Factory::operator-(s,a);},py::is_operator())
      .def("__ne__",[](const var<int>::Ptr& a,const var<int>::Ptr& b) { return Factory::operator!=(a,b);},py::is_operator())
      .def("__eq__",[](const var<int>::Ptr& a,const var<int>::Ptr& b) { return Factory::equal(a,b,0);},py::is_operator())
      .def("__ne__",[](const var<int>::Ptr& a,int b) { return Factory::operator!=(a,b);},py::is_operator())
      .def("__eq__",[](const var<int>::Ptr& a,int b) { return Factory::operator==(a,b);},py::is_operator())
      .def("__repr__",[](const var<int>& s) {
                         std::ostringstream str;
                         s.print(str);
                         str << std::ends;
                         return str.str();
                      });
   py::class_<var<bool>,AVar,var<bool>::Ptr>(m,"VarBool")
      .def(py::init<CPSolver::Ptr&>())
      .def("isTrue",&var<bool>::isTrue)
      .def("isFalse",&var<bool>::isFalse)
      .def("assign",&var<bool>::assign)
      .def("__repr__",[](const var<bool>& s) {
                         std::ostringstream str;
                         s.print(str);
                         str << std::ends;
                         return str.str();
                      });

   
   py::class_<CPSolver,CPSolver::Ptr>(m,"CPSolver")
      .def("incrNbChoices",&CPSolver::incrNbChoices)
      .def("incrNbSol",&CPSolver::incrNbSol)
      .def("fixpoint",&CPSolver::fixpoint)
      .def("post",&CPSolver::post,py::arg("c"),py::arg("enforceFixPoint")=true,"Post the constraint `c` and runs the fixpoint as required.")
      .def("__repr__",[](const CPSolver& s) {
                         std::ostringstream str;
                         str << s << std::ends;
                         return str.str();
                      });

   py::class_<Storage,Storage::Ptr>(m,"Storage");
   py::class_<stl::StackAdapter<var<int>::Ptr,Storage::Ptr>>(m,"Alloci");
   py::class_<EVec<var<int>::Ptr,stl::StackAdapter<var<int>::Ptr,Storage::Ptr>>>(m,"VecIntVar")
      .def("__getitem__",[](const Factory::Veci& s,size_t i) { return s[i];})
      .def("__setitem__",[](Factory::Veci& s,size_t i,var<int>::Ptr e) { s[i] = e;})
      .def("__len__",&Factory::Veci::size)
      .def("__iter__",[](const Factory::Veci& s) { return py::make_iterator(s.begin(),s.end());},py::keep_alive<0,1>())
      .def("__repr__",[](const Factory::Veci& s) {
                         std::ostringstream str;
                         str << s << std::ends;
                         return str.str();
                      });
   py::class_<EVec<var<bool>::Ptr,stl::StackAdapter<var<bool>::Ptr,Storage::Ptr>>>(m,"VecBoolVar")
      .def("__getitem__",[](const Factory::Vecb& s,size_t i) { return s[i];})
      .def("__setitem__",[](Factory::Vecb& s,size_t i,var<bool>::Ptr e) { s[i] = e;})
      .def("__len__",&Factory::Vecb::size)
      .def("__iter__",[](const Factory::Vecb& s) { return py::make_iterator(s.begin(),s.end());},py::keep_alive<0,1>())
      .def("__repr__",[](const Factory::Vecb& s) {
                         std::ostringstream str;
                         str << s << std::ends;
                         return str.str();
                      });

   py::class_<SearchStatistics>(m,"SearchStatistics")
      .def(py::init<>())
      .def("incrFailures",&SearchStatistics::incrFailures)
      .def("incrNodes",&SearchStatistics::incrNodes)
      .def("incrSolutions",&SearchStatistics::incrSolutions)
      .def("setCompleted",&SearchStatistics::setCompleted)
      .def("numberOfFailures",&SearchStatistics::numberOfFailures)
      .def("numberOfNodes",&SearchStatistics::numberOfNodes)
      .def("numberOfSolutions",&SearchStatistics::numberOfSolutions)
      .def("isCompleted",&SearchStatistics::isCompleted)
      .def("__repr__",[](const SearchStatistics& s) {
                         std::ostringstream str;
                         str << s << std::ends;
                         return str.str();
                      });      
      
   py::class_<DFSearch>(m,"DFSearch")
      .def(py::init<CPSolver::Ptr,std::function<Branches(void)>>())
      .def("onSolution",&DFSearch::onSolution<std::function<void(void)>>)
      .def("onFailure",&DFSearch::onFailure<std::function<void(void)>>)
      .def("solve",py::overload_cast<>(&DFSearch::solve))
      .def("solve",py::overload_cast<std::function<bool(const SearchStatistics&)>>(&DFSearch::solve))
      .def("solve",py::overload_cast<SearchStatistics&,std::function<bool(const SearchStatistics&)>>(&DFSearch::solve))
      .def("solveSubjectTo",&DFSearch::solveSubjectTo)
      .def("optimize",py::overload_cast<Objective::Ptr>(&DFSearch::optimize))
      .def("optimize",py::overload_cast<Objective::Ptr,std::function<bool(const SearchStatistics&)>>(&DFSearch::optimize))
      .def("optimizeSubjectTo",&DFSearch::optimizeSubjectTo)
      ;
   
   
   m.def("makeSolver",&Factory::makeSolver,"factory method");
   m.def("makeIntVar",py::overload_cast<CPSolver::Ptr,int,int>(&Factory::makeIntVar),"factory method");
   m.def("makeBoolVar",py::overload_cast<CPSolver::Ptr>(&Factory::makeBoolVar),"factory for boolean variables");
   m.def("intVarArray",(Factory::Veci (*)(CPSolver::Ptr,int,int,int))(&Factory::intVarArray),"factory method var<int>[]");
   m.def("intVarArray",(Factory::Veci (*)(CPSolver::Ptr,int,int))(&Factory::intVarArray),"factory method var<int>[]");
   m.def("intVarArray",(Factory::Veci (*)(CPSolver::Ptr,int))(&Factory::intVarArray),"factory method var<int>[]");
   m.def("boolVarArray",(Factory::Vecb (*)(CPSolver::Ptr,int))(&Factory::boolVarArray),"factory method var<bool>[]");

   m.def("equal",&Factory::equal);
   m.def("notEqual",&Factory::notEqual);

   m.def("firstFail",&firstFail<Factory::Veci>);
   m.def("selectMin",&selectMin<Factory::Veci,std::function<bool(var<int>::Ptr)>,std::function<int(var<int>::Ptr)>>);
}
