In this assignment you will implement a simple Healthcare management system in which an
administrator of the system can search for all appointments of a particular doctor. Also, the
administrator could add a doctor, delete a doctor, add an appointment and delete an
appointment record. 

When you add a record, first look at the AVAIL LIST, then write the record. If there
is a record available in the AVAIL LIST, write the record to a record AVAIL LIST
points and make appropriate changes on the AVAIL LIST.
o If the record to be added already exits, do not write that record to the file.
o When you delete a record, do not physically delete the record from file, just put a
marker (*) on the file and make appropriate changes on AVAIL LIST.
o If the record to be deleted does not exist, display a warning message on the screen.
o For the update function, make updates to non-key fields only. Also, updates to these
fields will not exceed the allocated size.
o Note: all add and delete operations will affect indexes.

Implement search operations. Make sure to consider the following:
o Search operations will use indexes (primary or secondary).
o All indexes are sorted ascending.
o Searching in indexes is performed using Binary search.
o Bind all secondary indexes with the primary index, don’t bind them by addresses
directly.
o You must implement secondary indexes using linked list technique.
❖ The user can write a query that contains specific key words (formatted in red below). Some
examples of user queries are as follows:
o Select all from Doctors where Doctor ID=’xxx’; // this query will use primary index to get the
results
o Select all from Appointments where Doctor ID=’xxx’; // this query will use secondary index to
get the results.
o Select Doctor Name from Doctors where Doctor ID=’xxx’; // this query will use secondary
index to get the results.
