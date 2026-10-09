
## Cluster Driver Specifications
### Cluster Open
```c
void* clusterOpen(pObject obj, int mask, pContentType sys_type, char* usr_type, pObjTrxTree* oxt);
```
`clusterOpen()` shall...
- Create or read a node, as indicated by passed flags.
  - Read flags from `obj->Mode`.
  - If `O_EXCL` is specified, `O_CREAT` is specified, and there are no other elements in the path, create a new node.
  - Otherwise attempt to read the previous object (in `obj->Prev`).
  - If this fails and `O_CREAT` is specified, create a new node.
  - If there is still no node, fail.
- Parse the provided path.
  - Use `obj_internal_PathPart()` with the pathname in `obj->Pathname`.
  - Not parse previous parts of the path already parsed by other drivers.
    - Start at the `obj->SubPtr`-th path element (skipping `obj->SubPtr - 1` elements).
  - Consume elements in the path until `obj_internal_PathPart()` returns `NULL`.
  - Store the number of elements consumed in `obj->SubCnt`.
- Determine what data is being targeted from the parsed path.
  - If the relevant part of the path contains only the name of the file, the driver shall set the target to root.
  - If it contains the name of a valid (sub)cluster or search, the driver shall set the target to that (sub)cluster or search.
  - Otherwise, the driver shall produce a descriptive error.
- Parse the provided structure file.
  - Follow the spec given in `cluster-schema.cluster`.
  - Produce descriptive errors when issues are detected.
- Return a new struct containing necessary information, including:
  - The name, source path, and attribute name.
  - All parameters (and a param list for scope), clusters, and searches.
    - Each parameter shall be represented by a `pParam` object (see `params.h`).
    - Each cluster shall be represented by a struct with information including:
      - Its name, clustering algorithm, and similarity measure.
      - The number of clusters to generate.
      - If a k-means algorithm is specified, the improvement threshold.
      - The maximum number of iterations to run.
      - A list of subclusters with at least this information for each.
    - Each search shall be represented by a struct with information including:
      - Its name, threshold, and similarity measure.
      - Its source, which is a valid cluster name of a cluster in the clusters list.
  - Information about targets, derived from the path.

### Cluster Close
```c
int clusterClose(void* inf_v, pObjTrxTree* oxt);
```
`clusterClose()` shall...
- Free all allocated data in the driver struct.
- Close any open files or the like in the driver struct.
- Return 0.

### Cluster Open Query
```c
void* clusterOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt);
```
`clusterOpenQuery()` shall...
- Return a query struct that can be passed to `clusterQueryFetch()`.
  - This struct shall contain an index to the last row accessed (starting at 0).
  - This struct shall contain a pointer to the driver data.

### Cluster Query Fetch
```c
void* clusterQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
```
`clusterQueryFetch()` shall...
- If the driver struct targets the root node, this function shall produce an error.
- If the driver struct targets an entry, this function shall produce a different error.
- If the driver targets a cluster or search, this function shall return a driver struct targetting the cluster or search *entry* (respectively) indicated by the query struct's row pointer, and increment the pointer.
  - Exception: If no data remains, this function shall return `NULL` instead.
  - This request shall cause clustering / searching to execute, if it has not executed already.

### Cluster Query Close
```c
int clusterQueryClose(void* qy_v, pObjTrxTree* oxt);
```
`clusterQueryClose()` shall...
- Free all allocated data in the query struct.
- Close any open files or the like in the query struct.
- Return 0.

### Cluster Get Attribute Type
```c
int clusterGetAttrType(void* qy_v, pObjTrxTree* oxt);
```
`clusterGetAttrType()` shall...
- Return the `DATA_T_...` type of the requested attribute, or `DATA_T_UNAVAILABLE` if the attribute does not exist.
- The name, content_type, inner_type, and outer_type attributes shall be of type `DATA_T_STRING`.
- The last_modification attribute shall be of type `DATA_T_DATETIME`.
- If the target is root...
  - The source and attr_name attributes shall be of type `DATA_T_STRING`.
- If the target is a cluster...
  - The algorithm and similarity_measure attributes shall be of type `DATA_T_STRING`.
  - The num_clusters and max_iterations attributes shall be of type `DATA_T_INTEGER`.
  - The improvement_threshold and average_similarity attributes shall be of type `DATA_T_DOUBLE`.
- If the target is a search...
  - The source and similarity_measure attribute shall be of type `DATA_T_STRING`.
  - The threshold attribute shall be of type `DATA_T_DOUBLE`.
- If the target is a cluster entry...
  - The val attribute shall be of type `DATA_T_INTEGER`.
  - The sim attribute shall be of type `DATA_T_DOUBLE`.
- If the target is a search entry...
  - The val1 and val2 attribute shall be of type `DATA_T_INTEGER`.
  - The sim attribute shall be of type `DATA_T_DOUBLE`.

### Cluster Get Attribute Value
```c
int clusterGetAttrValue(void* inf_v, char* attr_name, int datatype, pObjData val, pObjTrxTree* _);
```
`clusterGetAttrValue()` shall...
- If the given datatype does not match that returned from `clusterGetAttrType()`, the function shall produce an error.
- Requesting the name attribute shall produce the following values, depending on the target:
  - If the target is root, the name in the driver struct (aka. the one specified in the .cluster file) shall be produced.
  - If the target is a cluster or cluster entry, the name of the cluster shall be produced.
  - If the target is a search or search entry, the name of the search shall be produced.
- Requesting the annotation shall produce some string describing the driver.
- Requesting the outer_type shall produce "system/row".
- Requesting the inner_type or content_type shall produce "system/void". (All path elements are consumed.)
- If the target is root...
  - Requesting source shall produce the source path.
  - Requesting attr_name shall produce the attribute name.
- If the target is a cluster...
  - Requesting algorithm shall produce the name of the clustering algorithm.
  - Requesting similarity_measure shall produce the name of the similarity measure.
  - Requesting num_clusters shall produce the number of clusters.
  - Requesting max_iterations shall produce the maximum number of iterations.
  - Requesting improvement_threshold shall produce the minimum improvement threshold.
  - Requesting average_similarity shall produce the average size of clusters, running clustering / searching algorithms, if necessary.
- If the target is a search...
  - Requesting source shall produce the name of the source cluster for the search.
  - Requesting similarity_measure shall produce the name of the similarity measure.
  - Requesting threshold shall produce the filtering threshold.
- If the target is a cluster entry...
  - Requesting val shall produce the value of the data point in this cluster.
  - Requesting sim shall produce the similarity of the data point to the center of the cluster.
- If the target is a cluster entry...
  - Requesting val1 or val2 shall produce the first and second value (respectively)detected in this search.
  - Requesting sim shall produce the similarity of these two data points.
  

### Cluster Get First Attribute
```c
char* clusterGetFirstAttr(void* inf_v, pObjTrxTree oxt);
```
`clusterGetFirstAttr()` shall...
- Reset the current attribute index on the driver struct to 0.
- Return the value of invoking `clusterGetNextAttr()`.

### Cluster Get Next Attribute
```c
char* clusterGetNextAttr(void* inf_v, pObjTrxTree oxt);
```
`clusterGetNextAttr()` shall...
- Return the attribute name at the attribute index given by the driver struct in the list of attributes based on the target type.
- Return `NULL` if the end of the list has been reached.
- Increase the attribute index on the driver struct by 1.

- The attribute name list for a targetting root shall include "source" and "attr_name".
- The attribute name list for a targetting a cluster shall include "algorithm", "similarity_measure", "num_clusters", "improvement_threshold", and "max_iterations".
- The attribute name list for a targetting a search shall include "source", "threshold", and "similarity_measure".
- The attribute name list for a targetting a cluster entry shall include "val" and "sim".
- The attribute name list for a targetting a search entry shall include "val1", "val2", and "sim".

### Cluster Get Next Attribute
```c
int clusterInfo(void* inf_v, pObjectInfo info);
```
`clusterInfo()` shall...
- Provide the OBJ_INFO_F_CANT_ADD_ATTR flag.
- Provide the OBJ_INFO_F_CANT_HAVE_CONTENT flag.
- Provide the OBJ_INFO_F_NO_CONTENT flag.
- If the target is a root...
  - Provide the OBJ_INFO_F_CAN_HAVE_SUBOBJ flag.
  - Provide the OBJ_INFO_F_SUBOBJ_CNT_KNOWN flag.
  - Provide the OBJ_INFO_F_HAS_SUBOBJ flag if there is at least one cluster or search.
  - Provide the OBJ_INFO_F_NO_SUBOBJ flag otherwise.
  - Provide the total number of clusters and searches as the number of subobjects.
- If the target is a cluster...
  - Provide the OBJ_INFO_F_CAN_HAVE_SUBOBJ flag.
  - Provide the OBJ_INFO_F_HAS_SUBOBJ flag.
  - If the algorithm has been run, provide OBJ_INFO_F_SUBOBJ_CNT_KNOWN flag and the number of data points clustered as the number of subobjects.
- If the target is a search...
  - Provide the OBJ_INFO_F_CAN_HAVE_SUBOBJ flag.
  - If the algorithm has been run...
    - Provide OBJ_INFO_F_SUBOBJ_CNT_KNOWN flag and the number of elements found by the search as the number of subobjects.
    - Provide the OBJ_INFO_F_HAS_SUBOBJ flag if at least one element was found.
- If the target is a cluster entry or a search entry...
  - Provide the OBJ_INFO_F_CANT_HAVE_SUBOBJ flag.