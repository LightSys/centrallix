package org.lightsys.centrallix.objectsystem;

import java.util.List;

public interface ObjDriver {
    String getName(); // size 64
    List getRootContentTypes();
    int getCapabilities();
    Object Open();
    Object OpenChild();
    int Close();
    int Create();
    int Delete();
    int DeleteObj();
    Object OpenQuery();
    int QueryDelete();
    Object QueryFetch();
    Object QueryCreate();
    int QueryClose();
    int Read();
    int Write();
    int GetAttrType();
    int GetAttrValue();
    String GetFirstAttr();
    String GetNextAttr();
    int SetAttrValue();
    int AddAttr();
    Object OpenAttr();
    String GetFirstMethod();
    String GetNextMethod();
    int ExecuteMethod();
    ObjPresentationHints PresentationHints();
    int Info();
    int Commit();
    int GetQueryCoverageMask();
    int GetQueryIdentityPath();
}
