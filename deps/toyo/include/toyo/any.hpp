#ifndef __TOYO_ANY_HPP__
#define __TOYO_ANY_HPP__

#include <type_traits>
#include <typeinfo>

namespace toyo {

class bad_any_cast : public std::exception {
public:
  virtual const char * what() const noexcept {
    return "toyo::bad_any_cast: failed conversion using toyo::any::cast";
  }
};

class any {

 public:

  ~any() {
    if (content_) {
      delete content_;
      content_ = nullptr;
    }
  }

  any(): content_(nullptr) {}

  any(const any& v): content_(v.content_ ? v.content_->clone() : nullptr) {}

  any(any&& v): content_(v.content_) { v.content_ = nullptr; }

  template <typename ValueType>
  any(const ValueType& v): content_(new holder<typename std::remove_cv<typename std::decay<const ValueType>::type>::type>(v)) {}

  template <typename ValueType>
  any(ValueType&& v
    , typename std::enable_if<!std::is_same<any&, ValueType>::value>::type* = 0 // disable if value has type `any&`
    , typename std::enable_if<!std::is_const<ValueType>::value>::type* = 0 // disable if value has type `const ValueType&&`
  ): content_(new holder<typename std::decay<ValueType>::type>(static_cast<ValueType&&>(v))) {}

  any& swap(any& other) {
    placeholder* tmp = content_;
    content_ = other.content_;
    other.content_ = tmp;
    return *this;
  }

  any& operator=(const any& v) {
    any(v).swap(*this);
    return *this;
  }

  any& operator=(any&& v) {
    v.swap(*this);
    any().swap(v);
    return *this;
  }

  template <typename ValueType>
  any& operator=(const ValueType& v) {
    any(v).swap(*this);
    return *this;
  }

  template <typename ValueType>
  any& operator=(ValueType&& v) {
    any(static_cast<ValueType&&>(v)).swap(*this);
    return *this;
  }

  bool has_value() {
    return !content_;
  }

  void reset() {
    any().swap(*this);
  }

  const std::type_info& type() const {
    return content_ ? content_->type() : typeid(void);
  }

  template<typename ValueType>
  static ValueType* cast(any* operand) noexcept {
    return operand && operand->type() == typeid(ValueType)
      ? &(
          static_cast<any::holder<typename std::remove_cv<ValueType>::type>*>(operand->content_)->held_
        )
      : nullptr;
  }

  template<typename ValueType>
  static const ValueType* cast(const any* operand) noexcept {
    return any::cast<ValueType>(const_cast<any *>(operand));
  }

  template<typename ValueType>
  static ValueType cast(any& operand) {
      typedef typename std::remove_reference<ValueType>::type nonref;
      nonref * result = any::cast<nonref>(&operand);
      if(!result)
          throw bad_any_cast();

      // Attempt to avoid construction of a temporary object in cases when 
      // `ValueType` is not a reference. Example:
      // `static_cast<std::string>(*result);` 
      // which is equal to `std::string(*result);`
      typedef typename std::conditional<
          std::is_reference<ValueType>::value,
          ValueType,
          typename std::add_rvalue_reference<ValueType>::type
      >::type ref_type;

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4172) // "returning address of local variable or temporary" but *result is not local!
#endif
      return static_cast<ref_type>(*result);
#ifdef _MSC_VER
#   pragma warning(pop)
#endif
  }

  template<typename ValueType>
  static ValueType cast(const any& operand) {
    typedef typename std::remove_reference<ValueType>::type nonref;
    return any::cast<const nonref&>(const_cast<any&>(operand));
  }

  template<typename ValueType>
  static ValueType cast(any&& operand) {
    return any::cast<ValueType>(operand);
  }

  template<typename ValueType>
  static ValueType* cast_unsafe(any* operand) noexcept {
    return &(
      static_cast<any::holder<ValueType>*>(operand->content_)->held_
    );
  }

  template<typename ValueType>
  static const ValueType* cast_unsafe(const any* operand) noexcept {
    return any::cast_unsafe<ValueType>(const_cast<any*>(operand));
  }

 private:
  
  class placeholder {
   public:
    virtual ~placeholder() {}
    virtual placeholder* clone() const = 0 ;
    virtual const std::type_info& type() const = 0 ;
  };

  template <typename ValueType>
  class holder : public placeholder {
   public:
    holder(const ValueType& v): held_(v) {}
    holder(ValueType&& v): held_(static_cast<ValueType&&>(v)) {}

    virtual placeholder* clone() const {
      return new holder(held_);
    }

    virtual const std::type_info& type() const {
      return typeid(ValueType);
    }

   private:
    ValueType held_;
    holder& operator=(const holder&);
    friend class any;
  };

  placeholder* content_; 
};

inline void swap(any& lhs, any& rhs) noexcept {
  lhs.swap(rhs);
}

} // namespace toyo

#endif
