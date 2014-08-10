#ifndef MASKED_VECTOR_HPP_
#define MASKED_VECTOR_HPP_

#include <vector>
#include <boost/multiprecision/cpp_int.hpp>

// 参考: http://txt-txt.hateblo.jp/entry/2013/07/11/184157

template <class ElementType> class masked_vector{
public:
	typedef boost::multiprecision::cpp_int mask_type;
	
private:
	std::vector<ElementType> elements_;
	mask_type mask_;
	
public:
	masked_vector() : elements_(), mask_(0){}
	
	template <class ContainerType> masked_vector(const ContainerType & container)
	: elements_(container.begin(), container.end()),
	  mask_(0){}
	
	inline bool has(size_t pos) const{ return boost::multiprecision::bit_test(mask_, pos); }
	inline ElementType & operator[](size_t pos){ return elements_[pos]; }
	inline const ElementType & operator[](size_t pos) const{ return elements_[pos]; }
	inline size_t size() const{ return elements_.size(); }
	inline size_t length() const{ return elements_.size(); }
	inline mask_type mask() const{ return mask_; }
	inline void set_mask(mask_type new_mask){ mask_ = new_mask; }
	
	size_t mask_size() const{
		size_t result = 0;
		for(size_t i = 0; i < size(); ++i){
			result += (has(i) ? 1 : 0);
		}
		return result;
	}
	
	inline void push_back(const ElementType & elem){
		elements_.push_back(elem);
	}
	
	void next(){
		++mask_;
		boost::multiprecision::bit_unset(mask_, size());
	}
	
	inline bool emptymask(){
		return(mask_ == 0);
	}
	
	// 何番目の位置に出現するかを返す。
	// 出現しない場合は size() を返す。
	// マスクを問わず、元の集合に含まれていれば出現するとみなす。
	// 第2引数が与えられた場合、そこにマスクの値が格納される。
	size_t index_orig(const ElementType & key, bool & appearing) const{
		appearing = false;
		size_t i;
		for(i = 0; i < size(); ++i){
			if(key == elements_[i]){
				appearing = has(i);
				break;
			}
		}
		return i;
	}
	
	size_t index_orig(const ElementType & key) const{
		size_t i;
		for(i = 0; i < size(); ++i){
			if(key == elements_[i]){
				break;
			}
		}
		return i;
	}
	
	// 何番目の位置に出現するかを返す。
	// 出現しない場合は size() を返す。
	// ただし、マスクによって「含んでいる」と判定された場合のみ出現するとみなす。
	size_t index(const ElementType & key) const{
		size_t i;
		for(i = 0; i < size(); ++i){
			if(has(i) && key == elements_[i]) break;
		}
		return i;
	}
};

#endif // MASKED_VECTOR_HPP_
