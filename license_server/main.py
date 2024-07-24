# Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from uuid import UUID

app = FastAPI()

activated_licenses = set()

class LicenseRequest(BaseModel):
    license_key: str

@app.post("/activate")
async def activate_license(request: LicenseRequest):
    try:
        uuid = UUID(request.license_key, version=4)
        if request.license_key in activated_licenses:
            return {"activated": True}
        else:
            activated_licenses.add(request.license_key)
            return {"activated": True}, 201

    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid license key")

@app.post("/check")
async def check_license(request: LicenseRequest):
    try:
        uuid = UUID(request.license_key, version=4)
        if request.license_key in activated_licenses:
            return {"activated": True}
        else:
            return {"activated": False}

    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid license key")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="127.0.0.1", port=8000)
