#@desc Checking we can train symmetric and asymmetric PNT models
${TRACTOR} pnt-train TractName:genu DatasetName:data/pnt/pnt_train_data
${TRACTOR} peek data/pnt/pnt_train_data_model
${TRACTOR} pnt-train TractName:genu DatasetName:data/pnt/pnt_train_data AsymmetricModel:true
${TRACTOR} peek data/pnt/pnt_train_data_model
rm -f data/pnt/pnt_train_data_model.Rdata
