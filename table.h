#ifndef table_h
#define table_h

template<class I> class Table
{
public:
	Table(int iSize = 64);

	virtual ~Table();

	int Add(const I*);

	int Delete(int idx);
	int DeleteAll();

	bool SetOwnsElements(bool owns);
	bool GetOwnsElements();

	I* GetItem(int idx);
	I* operator[](int i);

	int GetCount();

protected:
	I** items;
	int	count;
	int max;
	bool owns;
};

template<class I> class SortTable : public Table<I>
{
public:
	SortTable(int iSize = 64) : Table<I>(iSize) {}

	void Sort();
};

template<class I> Table<I>::Table(int iSize)
{
	count = 0;
	max	  = iSize;
	owns  = false;
	items = new I*[max];
}

template<class I> Table<I>::~Table()
{
	if (owns)
		for (int i = 0; i < count; i++)
			delete items[i];

	delete[] items;	
}

template<class I> int Table<I>::Add(const I* pItem)
{
	if (count == max)
	{
		I** newitems = new I*[max * 2];
		memcpy(newitems, items, count * sizeof(I*));
		delete[] items;
		items = newitems;
		max *= 2;
	}
	items[count++] = (I*)pItem;
	return count - 1;
}

template<class I> int Table<I>::Delete(int idx)
{
	if (owns) delete items[idx];
	for (int i = idx; idx < count -  1; i++)
		items[i] = items[i + 1];
	count--;
	return count;
}

template<class I> int Table<I>::DeleteAll()
{
	if (owns)
		for (int i = 0; i < count; i++)
			delete items[i];
	count = 0;
	return 0;
}

template<class I> bool Table<I>::SetOwnsElements(bool o)
{
	owns = o;
	return true;
}

template<class I> bool Table<I>::GetOwnsElements()
{
	return owns;
}

template<class I> I* Table<I>::GetItem(int idx)
{
	return items[idx];
}

template<class I> I* Table<I>::operator[](int idx)
{
	return items[idx];
}

template<class I> int Table<I>::GetCount()
{
	return count;
}


template<class I> int cmp(const I** r1, const I** r2)
{
    if (**r1 <  **r2)
        return -1;
    if (**r1 == **r2)
    	return 0;
    return 1;
}

template<class I> void SortTable<I>::Sort()
{
    if (count<2) return;
    int (*pRf) (const I **, const I **) = cmp;
	int (*pvf) (const void *, const void *) = (int (*) (const void *, const void *)) pRf;
	qsort(items, count, 4, pvf);
}

#endif
